#include "trace_gep.h"

#include <getopt.h>

#include <cassert>
#include <chrono>
#include <deque>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <unordered_set>
#include <unordered_map>

#include <ginac/ginac.h>

#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/LazyValueInfo.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_os_ostream.h>

static const struct option options[] = {
	{"function", required_argument, NULL, 'f'},
	{"arg", required_argument, NULL, 'a'},
	{"probability", no_argument, NULL, 'p'},
	{"run-evaluation", no_argument, NULL, 'r'},
	{"evalution-start", required_argument, NULL, 's'},
	{"evalution-stop", required_argument, NULL, 't'},
	{"evalution-step", required_argument, NULL, 'e'},
	{"evalution-output", required_argument, NULL, 'o'},
	{"sample-counts", required_argument, NULL, 'c'},
	{"help", no_argument, NULL, 'h'},
};

using store_list = ::std::deque<::llvm::StoreInst*>;
using inst_set = ::std::unordered_set<::llvm::Instruction *>;
struct prob {
	GiNaC::ex expression;
	GiNaC::lst variables;
	// Use only the expression field if only one ginac expression is needed
	prob(const GiNaC::ex &e): expression(e), variables() {}
	prob(const GiNaC::ex &e, const GiNaC::ex &v): expression(e), variables({v}) {};
	prob(const GiNaC::ex &e, const GiNaC::lst &l): expression(e), variables(l) {};
};
using val_map = ::std::unordered_map<const ::llvm::Value *, prob>;

::llvm::raw_os_ostream llvm_cout(::std::cout);

static void get_stored_values(::llvm::Value *val, store_list &stored_vals) {
	if (::llvm::StoreInst *i = ::llvm::dyn_cast<::llvm::StoreInst>(val)) {
		llvm_cout << "Used in store:" << *i << "\n";
		stored_vals.push_front(i);
	}
	for (auto u:val->users())
		get_stored_values(u, stored_vals);
}

static store_list find_arg_values(::llvm::Function &f, unsigned arg) {
	// Run analysis pass on function
	llvm_cout << "Running on function: " << f.getName().str()
	           << " (" << f.arg_size() << " args)\n";
	if (arg >= f.arg_size()) {
		::std::cerr << "Argument " << arg << " not available. Function has only " << f.arg_size() << " arguments.\n";
		return store_list();
	}
	::llvm::Argument &output_arg = f.arg_begin()[arg];
	llvm_cout << "Argument " << arg << ": " << output_arg << "\n";
	store_list vals;
	get_stored_values(&output_arg, vals);
	if (vals.empty())
		llvm_cout << "No stores to selected location.\n";
	llvm_cout.flush();
	return vals;
}

struct config {
	::std::string func = "run_";
	unsigned arg = 4;
	bool prob = false;
	bool run_eval = false;
	double eval_start = -10.0;
	double eval_stop = 10.0;
	double eval_step = 0.1;
	::std::string eval_out = "eval_out.gp";
	unsigned sim_count = 1000000;
};

static const char * get_sym(void) {
	static unsigned char count = 0;
	static const char *table[] = {"a", "b", "c" , "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u" ,"v", "w", "x", "y", "z"};
	return table[count++ % (sizeof(table) / sizeof(table[0]))];
}

static void add_results(const ::llvm::Value *val, val_map &store)
{
	if (auto *C = ::llvm::dyn_cast<::llvm::ConstantFP>(val)) {
		double Value = C->getValueAPF().convertToDouble();
		store.insert({val, {Value}});
		return;
	}
	if (auto *C = ::llvm::dyn_cast<::llvm::ConstantInt>(val)) {
		int64_t Value = C->getValue().getLimitedValue();
		store.insert({val, {Value}});
		return;
	}

	const ::llvm::Instruction *I = ::llvm::cast<::llvm::Instruction>(val);
	switch (I->getOpcode()) {
	case ::llvm::Instruction::Load: {
		::GiNaC::symbol s(get_sym());
		store.insert({val, {s, s}});
		break;
	}
	case ::llvm::Instruction::FAdd: {
		::GiNaC::ex expr = store.at(I->getOperand(0)).expression +
		                   store.at(I->getOperand(1)).expression;
		store.insert({val, expr});
		break;
	}
	case ::llvm::Instruction::FSub: {
		::GiNaC::ex expr = store.at(I->getOperand(0)).expression -
		                   store.at(I->getOperand(1)).expression;
		store.insert({val, expr});
		break;
	}
	case ::llvm::Instruction::FMul: {
		::GiNaC::ex expr = store.at(I->getOperand(0)).expression *
		                   store.at(I->getOperand(1)).expression;
		store.insert({val, expr});
		break;
	}
	case ::llvm::Instruction::FDiv: {
		::GiNaC::ex expr = store.at(I->getOperand(0)).expression /
		                   store.at(I->getOperand(1)).expression;
		store.insert({val, expr});
		break;
	}
	case ::llvm::Instruction::Call: {
		const ::llvm::CallInst *CI = ::llvm::cast<::llvm::CallInst>(I);
		switch (CI->getCalledFunction()->getIntrinsicID()) {
		case ::llvm::Intrinsic::exp: {
			::GiNaC::ex expr = ::GiNaC::exp(store.at(CI->getArgOperand(0)).expression);
			store.insert({val, expr});
			break;
		}
		default:
			llvm_unreachable("Unsupported intrinsic");
		}
		break;
	}
	case ::llvm::Instruction::FCmp: {
		::GiNaC::ex expr = ::GiNaC::symbol("X");
		const ::llvm::FCmpInst *FCI = ::llvm::cast<::llvm::FCmpInst>(I);
		::GiNaC::ex LHS = store.at(FCI->getOperand(0)).expression;
		::GiNaC::ex RHS = store.at(FCI->getOperand(1)).expression;
		switch (FCI->getPredicate()) {
		case ::llvm::CmpInst::FCMP_OEQ:
		case ::llvm::CmpInst::FCMP_UEQ:
			// step(x) * step(-x): x != 0 -> 0; x == 0 -> 1/4
			expr = ::GiNaC::step(LHS - RHS) * ::GiNaC::step(RHS - LHS) * 4;
			break;
		case ::llvm::CmpInst::FCMP_ONE:
		case ::llvm::CmpInst::FCMP_UNE:
			// (step(x) - 0.5) * (step(-x) - 0.5): x == 0 -> 0; x != 0 -> -1/4
			expr = (::GiNaC::step(LHS - RHS) - 0.5) *
			       (::GiNaC::step(RHS - LHS) - 0.5) * -4;
			break;
		case ::llvm::CmpInst::FCMP_OLT:
		case ::llvm::CmpInst::FCMP_ULT:
			// step(step(x - 0.75)): x <= 0 -> 0; x > 0 -> 1
			expr = ::GiNaC::step(::GiNaC::step(RHS - LHS) - 0.75);
			break;
		case ::llvm::CmpInst::FCMP_OLE:
		case ::llvm::CmpInst::FCMP_ULE:
			// step(step(x - 0.25)): x < 0 -> 0; x => 0 -> 1
			expr = ::GiNaC::step(::GiNaC::step(RHS - LHS) - 0.25); break;
		case ::llvm::CmpInst::FCMP_OGT:
		case ::llvm::CmpInst::FCMP_UGT:
			// step(step(x - 0.75)): x <= 0 -> 0; x > 0 -> 1
			expr = ::GiNaC::step(::GiNaC::step(LHS - RHS) - 0.75);
			break;
		case ::llvm::CmpInst::FCMP_OGE:
		case ::llvm::CmpInst::FCMP_UGE:
			// step(step(x)): x < 0 -> 0; x => 0 -> 1
			expr = ::GiNaC::step(::GiNaC::step(LHS - RHS)); break;
		default:
			llvm_unreachable("Unexpected FCMP ppredicate");
		}
		store.insert({val, expr});
		break;
	}
	case ::llvm::Instruction::Select: {
		const ::llvm::SelectInst *SI = ::llvm::cast<::llvm::SelectInst>(I);
		::GiNaC::ex CondVal = store.at(SI->getCondition()).expression;
		::GiNaC::ex TrueVal = store.at(SI->getTrueValue()).expression;
		::GiNaC::ex FalseVal = store.at(SI->getFalseValue()).expression;
		::GiNaC::ex res = CondVal * TrueVal + ((1 - CondVal) * FalseVal);
		store.insert({val, res});
		break;
	}
	default:
		llvm_unreachable("Unsupported instruction");
	}
}

static prob get_normal_dist(double u, double s2)
{
	::GiNaC::symbol x(get_sym());
	::GiNaC::ex expr = (1 / ::GiNaC::sqrt(2 * ::GiNaC::Pi * s2)) * ::GiNaC::exp(((x - u) * (x - u) / (-2 * s2)));
	return {expr, x};
}

static void add_prob_results(const ::llvm::Value *val, val_map &store)
{
	if (auto *C = ::llvm::dyn_cast<::llvm::ConstantFP>(val)) {
		double Value = C->getValueAPF().convertToDouble();
		store.insert({val, {Value, Value}});
		return;
	}
	if (auto *C = ::llvm::dyn_cast<::llvm::ConstantInt>(val)) {
		int64_t Value = C->getValue().getLimitedValue();
		store.insert({val, {Value, Value}});
		return;
	}

	const ::llvm::Instruction *I = ::llvm::cast<::llvm::Instruction>(val);
	switch (I->getOpcode()) {
	case ::llvm::Instruction::Load: {
		// Assume X ~ N(0, 1)
		store.insert({val, get_normal_dist(0, 1)});
		break;
	}
	case ::llvm::Instruction::FAdd: {
		auto LHS = store.at(I->getOperand(0));
		auto RHS = store.at(I->getOperand(1));
		assert(LHS.variables.nops() == 1);
		assert(RHS.variables.nops() == 1);
		::GiNaC::symbol new_var(I->getName().str());
		if (::GiNaC::is_a<::GiNaC::numeric>(LHS.expression)) {
			double LHSVal = ::GiNaC::ex_to<::GiNaC::numeric>(LHS.expression).to_double();
			::GiNaC::ex expr = RHS.expression.subs(RHS.variables[0] == (new_var - LHSVal));
			store.insert({val, {expr, new_var}});
			break;
		}
		if (::GiNaC::is_a<::GiNaC::numeric>(RHS.expression)) {
			double RHSVal = ::GiNaC::ex_to<::GiNaC::numeric>(RHS.expression).to_double();
			::GiNaC::ex expr = LHS.expression.subs(LHS.variables[0] == (new_var - RHSVal));
			store.insert({val, {expr, new_var}});
			break;
		}
	}
	default:
		llvm_unreachable("Unsupported instruction");
	}
}

static val_map find_symbolic(inst_set fringe, void(*f)(const ::llvm::Value *val, val_map &store))
{
	val_map results;
	inst_set deps;

	do {
		fringe.merge(deps);
		while (!fringe.empty()) {
			auto val = *fringe.begin();
			fringe.erase(val);
			bool all_ready = true;
			for (auto &op:val->operands()) {
				::llvm::Value *op_val = op.get();
				// Ignore pointers
				if (op_val->getType()->isPointerTy())
					continue;
				// Constants are trivially computed
				if (::llvm::isa<::llvm::Constant>(op_val)) {
					f(op_val, results);
					continue;
				}

				// The operand is already available
				if (results.count(op_val) > 0)
					continue;

				::llvm::Instruction *i =
				  ::llvm::cast<::llvm::Instruction>(op_val);
				all_ready = false;
				fringe.insert(i);
			}
			if (all_ready)
				f(val, results);
			else
				deps.insert(val);
		}
	} while (!deps.empty());
	return results;
}

static void run_eval(const config &conf, const prob &p, const prob &f,
                     const ::llvm::StoreInst *s);

static void analyze_function(::llvm::Function &f, const config &conf)
{
	auto store_vals = find_arg_values(f, conf.arg);
	inst_set fringe;
	for (auto s:store_vals) {
		auto val = s->getValueOperand();
		if (::llvm::Instruction *i = ::llvm::dyn_cast<::llvm::Instruction>(val))
			fringe.insert(i);
	}

	auto res = find_symbolic(fringe, add_results);
	auto prob_res = find_symbolic(fringe, add_prob_results);
	for (auto v:store_vals) {
		// Pointer is the second arg for stores
		::llvm::Value *ptr = v->getPointerOperand();
		::llvm::GetElementPtrInst *gep =
			::llvm::dyn_cast<::llvm::GetElementPtrInst>(ptr);
		::llvm::AllocaInst *i;
		auto idx_seq = trace_gep(gep, i);
		for (const auto idx:idx_seq)
			::std::cout << idx->getValue().getLimitedValue() << " ";
		auto res_val = res.at(v->getValueOperand());
		auto prob_res_val = prob_res.at(v->getValueOperand());
		::std::cout << v->getValueOperand()->getName().str() << ": ";
		if (conf.prob)
		        ::std::cout << prob_res_val.expression
			            << " VAR: " << prob_res_val.variables;
		else
		        ::std::cout << res_val.expression;
		::std::cout << "\n";
		if (conf.run_eval)
			run_eval(conf, prob_res_val, res_val, v);
	}
}

int main(int argc, char **argv) {
	char c = -1;
	config conf;
	while ((c = getopt_long(argc, argv, "f:a:prs:t:e:o:c:h", options, NULL)) != -1) {
		switch (c) {
		case 'f': conf.func = ::std::string(optarg); break;
		case 'a': conf.arg = ::std::stoi(optarg); break;
		case 'p': conf.prob = true; break;
		case 'r': conf.run_eval = true; break;
		case 's': conf.eval_start = ::std::stod(optarg); break;
		case 't': conf.eval_stop = ::std::stod(optarg); break;
		case 'e': conf.eval_step = ::std::stod(optarg); break;
		case 'o': conf.eval_out = optarg; break;
		case 'c': conf.sim_count = ::std::stoi(optarg); break;
		default:
			::std::cerr << "Unknown option: " << argv[optind - 1]
			            << ::std::endl;
		case 'h':
			::std::cerr << "Available options:\n";
			::std::cerr << "\t\t-f,--function <f> Function to examine.\n";
			::std::cerr << "\t\t-a,--arg <a> Function output argument.\n";
			::std::cerr << "\t\t-p,--probability Use probability distributions instead of symbolic variables\n";
			::std::cerr << "\t\t-e,--run-evaluation Evaluate inferred PDF\n";
			::std::cerr << "\t\t-s,--evaluation-start X axis minimum (default: -10.0)\n";
			::std::cerr << "\t\t-t,--evaluation-stop X axis maximum (default: 10.0)\n";
			::std::cerr << "\t\t-e,--evaluation-step X axis step 9default: 0.1)\n";
			::std::cerr << "\t\t-o,--evaluation-output-file filename to output gnuplot script (default: 'eval_out.gp')\n";
			::std::cerr << "\t\t-o,--sample-count Number of samples to use for evaluation (Default: 1000000)\n";
			return c == 'h' ? 0 : 1;
		}
	}

	static ::llvm::LLVMContext context;
	::std::deque<::std::unique_ptr<::llvm::Module>> modules;
	for (int i = optind; i < argc; ++i) {
		::std::cout << "Parsing file: " << argv[i] << "\n";
		::llvm::raw_os_ostream errs(::std::cerr);
		::llvm::SMDiagnostic error;
		auto ptr = ::llvm::parseIRFile(argv[i], error, context);
		if (ptr) {
			modules.push_back(::std::move(ptr));
		} else {
			error.print(argv[i], errs);
		}
	}

	for (auto &m:modules) {
		for (auto &f: m->functions()) {
			auto mm = ::std::mismatch(conf.func.begin(),
			                          conf.func.end(),
						  f.getName().begin());
			if (mm.first == conf.func.end())
				analyze_function(f, conf);
		}
	}

	return 0;
}

class SymVisitor :
	public ::GiNaC::visitor,
	public ::GiNaC::symbol::visitor {

	void visit(const ::GiNaC::symbol &s)
	{ symbols.push_back(&s); }
public:
	::std::deque<const ::GiNaC::symbol*> symbols;

};

static ::std::vector<double> run_sampling(const config &conf, const prob &f,
                                          const ::llvm::StoreInst *s)
{
	// TODO: Is there an easier way to get these?
	SymVisitor symbols;
	f.expression.traverse(symbols);

	unsigned buckets = (conf.eval_stop - conf.eval_start) / conf.eval_step;
	::std::vector<double> results(buckets);
	::std::default_random_engine generator;
	::std::normal_distribution<double> distribution(0.0, 1.0);
	auto start = ::std::chrono::high_resolution_clock::now();
	for (unsigned i = 0; i < conf.sim_count; ++i) {
		::GiNaC::lst l;
		for (const auto &s: symbols.symbols) {
			double val = distribution(generator);
			l.append(*s == val);
		}
		auto res = ::GiNaC::evalf(f.expression.subs(l));
		double res_d = ::GiNaC::ex_to<::GiNaC::numeric>(res).to_double();
		if (res_d >= conf.eval_start && res_d < conf.eval_stop) {
			unsigned bucket = (res_d - conf.eval_start) / conf.eval_step;
			results[bucket] += 1.0;
		}
	}
	auto end = ::std::chrono::high_resolution_clock::now();
	auto dur = ::std::chrono::duration_cast<::std::chrono::microseconds>(end - start);
	::std::cout << "Sampling evaluation took: " << dur.count() << " us\n";

	return results;
}

static void run_eval(const config &conf, const prob &p, const prob &orig,
                     const ::llvm::StoreInst *s)
{
	assert(conf.eval_stop > conf.eval_start);
	assert(conf.eval_step > 0);
	assert(s);

	unsigned iters = (conf.eval_stop - conf.eval_start) / conf.eval_step;
	::std::vector<::std::pair<double, double> > data;
	data.reserve(iters);
	auto start = ::std::chrono::high_resolution_clock::now();
	for (unsigned i = 0; i < iters; ++i) {
		double val = conf.eval_start + (conf.eval_step * i);
		auto res = ::GiNaC::evalf(p.expression.subs(p.variables[0] == val));
		double res_d = ::GiNaC::ex_to<::GiNaC::numeric>(res).to_double();
		data.emplace_back(val, res_d);
	}
	auto end = ::std::chrono::high_resolution_clock::now();
	auto dur = ::std::chrono::duration_cast<::std::chrono::microseconds>(end - start);
	::std::cout << "PDF Function evaluation took: " << dur.count() << " us\n";
	const auto sampled_data = run_sampling(conf, orig, s);

	// Store data in a gnuplot script
	::std::ofstream f(conf.eval_out, ::std::ios_base::out | ::std::ios_base::trunc);
	f << "$sampled_data << EOD\n";
	assert(sampled_data.size() == iters);
	for (unsigned i = 0; i < iters; ++i)
		f << conf.eval_start + i * conf.eval_step << "\t"
		  << sampled_data[i] << "\n";
	f << "EOD\n\n";
	f << "# Time to generate PDF data: " << dur.count() << " usecs\n";
	f << "$data << EOD\n";
	for (const auto &d: data)
		f << d.first << "\t" << d.second << "\n";
	f << "EOD\n\n";
	f << "set title \"" << s->getFunction()->getFunction().getName().str()
	  << " (" << conf.sim_count << " samples)\" noenhanced\n";
	f << "plot \"$data\" with lines title \""
	  << s->getValueOperand()->getName().str() << "\", \\\n";
	f << "\t\"$sampled_data\" axes x1y2 with boxes title \"sampled_"
	  << s->getValueOperand()->getName().str() << "\" noenhanced\n";
	f.close();
}
