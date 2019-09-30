#include "trace_gep.h"

#include <getopt.h>

#include <cassert>
#include <deque>
#include <iostream>
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
	{"no-signed-zeros", no_argument, NULL, 'z'},
	{"verbose", no_argument, NULL, 'v'},
	{"help", no_argument, NULL, 'h'},
};

using store_list = ::std::deque<::llvm::StoreInst*>;
using inst_set = ::std::unordered_set<::llvm::Instruction *>;
using val_map = ::std::unordered_map<const ::llvm::Value *, GiNaC::ex>;

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
};

static const char * get_sym(void) {
	static unsigned char count = 0;
	static const char *table[] = {"a", "b", "c" , "d", "e", "f", "g", "h"};
	return table[count++ % (sizeof(table) / sizeof(table[0]))];
}

static void add_results(const ::llvm::Value *val, val_map &store)
{
	if (auto *C = ::llvm::dyn_cast<::llvm::ConstantFP>(val)) {
		double Value = C->getValueAPF().convertToDouble();
		store.insert(::std::make_pair(val, Value));
		return;
	}
	if (auto *C = ::llvm::dyn_cast<::llvm::ConstantInt>(val)) {
		int64_t Value = C->getValue().getLimitedValue();
		store.insert(::std::make_pair(val, Value));
		return;
	}

	const ::llvm::Instruction *I = ::llvm::cast<::llvm::Instruction>(val);
	switch (I->getOpcode()) {
	case ::llvm::Instruction::Load: {
		::GiNaC::symbol s(get_sym());
		store.insert(::std::make_pair(val, s));
		break;
	}
	case ::llvm::Instruction::FAdd: {
		::GiNaC::ex expr = store.at(I->getOperand(0)) +
		                   store.at(I->getOperand(1));
		store.insert(::std::make_pair(val, expr));
		break;
	}
	case ::llvm::Instruction::FSub: {
		::GiNaC::ex expr = store.at(I->getOperand(0)) -
		                   store.at(I->getOperand(1));
		store.insert(::std::make_pair(val, expr));
		break;
	}
	case ::llvm::Instruction::FMul: {
		::GiNaC::ex expr = store.at(I->getOperand(0)) *
		                   store.at(I->getOperand(1));
		store.insert(::std::make_pair(val, expr));
		break;
	}
	case ::llvm::Instruction::FDiv: {
		::GiNaC::ex expr = store.at(I->getOperand(0)) /
		                   store.at(I->getOperand(1));
		store.insert(::std::make_pair(val, expr));
		break;
	}
	case ::llvm::Instruction::Call: {
		const ::llvm::CallInst *CI = ::llvm::cast<::llvm::CallInst>(I);
		switch (CI->getIntrinsicID()) {
		case ::llvm::Intrinsic::exp: {
			::GiNaC::ex expr = ::GiNaC::exp(store.at(CI->getArgOperand(0)));
			store.insert(::std::make_pair(val, expr));
			break;
		}
		default:
			store.insert(::std::make_pair(val, ::GiNaC::symbol("UNK_CALL")));
		}
		break;
	}
	case ::llvm::Instruction::FCmp: {
		::GiNaC::ex expr = ::GiNaC::symbol("X");
		const ::llvm::FCmpInst *FCI = ::llvm::cast<::llvm::FCmpInst>(I);
		::GiNaC::ex LHS = store.at(FCI->getOperand(0));
		::GiNaC::ex RHS = store.at(FCI->getOperand(1));
		switch (FCI->getPredicate()) {
		case ::llvm::CmpInst::FCMP_OEQ:
		case ::llvm::CmpInst::FCMP_UEQ:
			expr = (LHS == RHS); break;
		case ::llvm::CmpInst::FCMP_ONE:
		case ::llvm::CmpInst::FCMP_UNE:
			expr = (LHS != RHS); break;
		case ::llvm::CmpInst::FCMP_OLT:
		case ::llvm::CmpInst::FCMP_ULT:
			expr = (LHS < RHS); break;
		case ::llvm::CmpInst::FCMP_OLE:
		case ::llvm::CmpInst::FCMP_ULE:
			expr = (LHS <= RHS); break;
		case ::llvm::CmpInst::FCMP_OGT:
		case ::llvm::CmpInst::FCMP_UGT:
			expr = (LHS > RHS); break;
		case ::llvm::CmpInst::FCMP_OGE:
		case ::llvm::CmpInst::FCMP_UGE:
			expr = (LHS >= RHS); break;
		default:
			llvm_unreachable("Unexpected FCMP ppredicate");
		}
		store.insert(::std::make_pair(val, expr));
		break;
	}
	case ::llvm::Instruction::Select: {
		const ::llvm::SelectInst *SI = ::llvm::cast<::llvm::SelectInst>(I);
		::GiNaC::ex CondVal = store.at(SI->getCondition());
		::GiNaC::ex TrueVal = store.at(SI->getTrueValue());
		::GiNaC::ex FalseVal = store.at(SI->getFalseValue());
		::GiNaC::ex res = CondVal * TrueVal + ((1 - CondVal) * FalseVal);
		store.insert(::std::make_pair(val, res));
		break;
	}
	default:
		store.insert(::std::make_pair(val, ::GiNaC::symbol("UNKN_I")));
	}
}

static val_map find_symbolic(inst_set &fringe)
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
					add_results(op_val, results);
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
				add_results(val, results);
			else
				deps.insert(val);
		}
	} while (!deps.empty());
	return results;
}

static void analyze_function(::llvm::Function &f, const config &conf)
{
	auto store_vals = find_arg_values(f, conf.arg);
	inst_set fringe;
	for (auto s:store_vals) {
		auto val = s->getValueOperand();
		if (::llvm::Instruction *i = ::llvm::dyn_cast<::llvm::Instruction>(val))
			fringe.insert(i);
	}

	auto res = find_symbolic(fringe);
	for (auto v:store_vals) {
		// Pointer is the second arg for stores
		::llvm::Value *ptr = v->getPointerOperand();
		::llvm::GetElementPtrInst *gep =
			::llvm::cast<::llvm::GetElementPtrInst>(ptr);
		::llvm::AllocaInst *i;
		auto idx_seq = trace_gep(gep, i);
		for (const auto idx:idx_seq)
			::std::cout << idx->getValue().getLimitedValue() << " ";
		::std::cout << ": " << res.at(v->getValueOperand()) << "\n";
	}
}

int main(int argc, char **argv) {
	char c = -1;
	config conf;
	while ((c = getopt_long(argc, argv, "f:a:h", options, NULL)) != -1) {
		switch (c) {
		case 'f': conf.func = ::std::string(optarg); break;
		case 'a': conf.arg = ::std::stoi(optarg); break;
		default:
			::std::cerr << "Unknown option: " << argv[optind - 1]
			            << ::std::endl;
		case 'h':
			::std::cerr << "Available options:\n";
			::std::cerr << "\t\t-f,--function <f> Function to examine.\n";
			::std::cerr << "\t\t-a,--arg <a> Function output argument.\n";
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
