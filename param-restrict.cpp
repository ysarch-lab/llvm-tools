#include "trace_gep.h"

#include <getopt.h>

#include <cassert>
#include <deque>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>

static const struct option options[] = {
	{"param", required_argument, NULL, 'p'},
	{"arg", required_argument, NULL, 'a'},
	{"function", required_argument, NULL, 'f'},
	{"lower-bound", required_argument, NULL, 'l'},
	{"upper-bound", required_argument, NULL, 'u'},
	{"help", no_argument, NULL, 'h'},
};

struct config {
	int arg = -1;
	::std::string function;
	::std::string param;
	::std::deque<double> lower_bound;
	::std::deque<double> upper_bound;
};

struct float_trait {
	static const ::llvm::CmpInst::Predicate EQ = ::llvm::CmpInst::FCMP_OEQ;
	static const ::llvm::CmpInst::Predicate LT = ::llvm::CmpInst::FCMP_OLT;
	static const ::llvm::CmpInst::Predicate GE = ::llvm::CmpInst::FCMP_OGE;
	template<typename B>
	static ::llvm::Value *build_cmp(B &builder, ::llvm::Value *val, double limit,
					::llvm::CmpInst::Predicate p, const char *n)
	{
		return builder.CreateFCmp(p, val,
		    ::llvm::ConstantFP::get(val->getType(), limit), n);
	}
};

struct int_trait {
	static const ::llvm::CmpInst::Predicate EQ = ::llvm::CmpInst::ICMP_EQ;
	static const ::llvm::CmpInst::Predicate LT = ::llvm::CmpInst::ICMP_SLT;
	static const ::llvm::CmpInst::Predicate GE = ::llvm::CmpInst::ICMP_SGE;
	template<typename B>
	static ::llvm::Value *build_cmp(B &builder, ::llvm::Value *val, int64_t limit,
					::llvm::CmpInst::Predicate p, const char *n)
	{
		return builder.CreateICmp(p, val,
		    ::llvm::ConstantInt::get(val->getType(), limit), n);
	}
};


template<typename T, typename B, typename C>
static void insert_assume(B &builder, ::llvm::Value *val, C lower, C upper)
{
	// Get assume Intrinsic
	::llvm::Intrinsic::ID id =
	  ::llvm::Function::lookupIntrinsicID("llvm.assume");
	::llvm::Function *assume_decl =
	  ::llvm::Intrinsic::getDeclaration(
	    builder.GetInsertBlock()->getParent()->getParent(), id);

	if (lower == upper) {
		::llvm::Value *eq_cond =
		    T::build_cmp(builder, val, upper, T::EQ, "eq_bound");
		builder.CreateCall(assume_decl->getFunctionType(), assume_decl, {eq_cond});
	} else {
		assert(lower < upper);

		::llvm::Value *upper_cond =
		    T::build_cmp(builder, val, upper, T::LT, "upper_bound");
		::llvm::Value *lower_cond =
		    T::build_cmp(builder, val, lower, T::GE, "lower_bound");

		builder.CreateCall(assume_decl->getFunctionType(), assume_decl, {upper_cond});
		builder.CreateCall(assume_decl->getFunctionType(), assume_decl, {lower_cond});
	}
}

template<typename B>
static void apply_assumption(B &builder, ::llvm::LoadInst *param_val,
		             const ::std::deque<double> &lower,
			     const ::std::deque<double> &upper)
{
	::llvm::Type *param_type = param_val->getType();

	// Create bound conditions
	switch (param_type->getTypeID()) {
	case ::llvm::Type::ArrayTyID: {
		const int64_t num_elements = param_type->getArrayNumElements();
		assert(num_elements == upper.size());
		assert(num_elements == lower.size());
		for (unsigned i = 0; i < num_elements; ++i) {
			::llvm::Value *element_val =
			    builder.CreateExtractValue(param_val, {i}, "extract_param_val");
			if (element_val->getType()->isFloatingPointTy())
				insert_assume<float_trait>(builder, element_val, lower[i], upper[i]);
			else
				insert_assume<int_trait>(builder, element_val, lower[i], upper[i]);
		}
		break;
	}
	case ::llvm::Type::IntegerTyID:
		insert_assume<int_trait>(builder, param_val, lower[0], upper[0]);
		break;
	case ::llvm::Type::FloatTyID:
	case ::llvm::Type::DoubleTyID:
		insert_assume<float_trait>(builder, param_val, lower[0], upper[0]);
		break;
	default:
		assert(!"Not implemented!");
	}
}

static void apply_param_assumption(::llvm::Instruction *src, const int_seq &indices,
		                   const ::std::deque<double> &lower,
			           const ::std::deque<double> &upper)
{
	::std::cerr << "Source: " << src->getName().str() << "\n";
	::std::cerr << "Found indices:";
	for (auto *idx:indices)
		::std::cerr << " " << idx->getZExtValue() << ",";
	::std::cerr << "\n";

	::llvm::StoreInst * store_val = nullptr;
	for (auto *user:src->users()) {
		if (::llvm::isa<::llvm::StoreInst>(user)) {
			assert(store_val == nullptr);
			store_val = ::llvm::cast<::llvm::StoreInst>(user);
		}
	}
	assert(store_val != nullptr);

	::llvm::Function *f = src->getParent()->getParent();
	::std::cerr << f->getName().str() << "\n";

	::std::vector<::llvm::Value *> llvm_indices(indices.size());
	::std::copy(indices.begin(), indices.end(), llvm_indices.begin());
	::llvm::Argument *param_arg = f->arg_begin() + 1;

	// Create builder and position after parameters set
	::llvm::IRBuilder<> builder(store_val->getNextNode());
	::llvm::Value *orig_gep =
	  builder.CreateInBoundsGEP(param_arg, llvm_indices, "orig_gep");
	::llvm::Value *alloca_gep =
	  builder.CreateInBoundsGEP(src, llvm_indices, "dst_gep");

	::llvm::LoadInst *load_orig = builder.CreateLoad(orig_gep, "load_orig");
	::llvm::Value *store_orig = builder.CreateStore(load_orig, alloca_gep);

	apply_assumption(builder, load_orig, lower, upper);
}

static void apply_module_param_assumption(::llvm::Module *m,
				          const config &conf)
{
	auto res = find_src_and_indices(m, conf.param);
	if (!res.second.empty()) {
		apply_param_assumption(res.first, res.second, conf.lower_bound, conf.upper_bound);
	}
}

static void apply_function_arg_assumption(::llvm::Function &f,
				          const config &conf)
{
	if (f.getName() != conf.function)
		return;
	::std::cerr << "Applying to function: " << conf.function << "\n";

	assert(f.arg_size() > conf.arg);
	auto &bb = f.getEntryBlock();
	::llvm::IRBuilder<> builder(&*bb.getFirstInsertionPt());
	::llvm::Argument *arg = f.arg_begin() + conf.arg;
	::llvm::LoadInst *val = builder.CreateLoad(arg, "restricted_arg_load");

	apply_assumption(builder, val, conf.lower_bound, conf.upper_bound);
}

static ::std::deque<double> parse_limits(const char *arg)
{
	::std::deque<double> limits;
	std::stringstream ss(arg);

	for (int i; ss >> i;) {
	        limits.push_back(i);
		if (ss.peek() == ',')
			ss.ignore();
	}
	return limits;
}

int main(int argc, char **argv) {
	config conf;
	char c = -1;
	while ((c = getopt_long(argc, argv, "sp:l:u:a:f:h", options, NULL)) != -1) {
		switch (c) {
		case 'p': conf.param = ::std::string(optarg); break;
		case 'a': conf.arg = ::std::stoi(optarg); break;
		case 'f': conf.function = ::std::string(optarg); break;
		case 'l': conf.lower_bound = parse_limits(optarg); break;
		case 'u': conf.upper_bound = parse_limits(optarg); break;
		default:
			::std::cerr << "Unknown option: " << argv[optind - 1]
			            << ::std::endl;
		case 'h':
			::std::cerr << "Available options:\n";
			::std::cerr << "\t-p,--param\t\tParameter name\n";
			::std::cerr << "\t-l,--lower-bound\t\tParameter lower bound\n";
			::std::cerr << "\t-u,--upper-bound\t\tParameter upper bound\n";
			::std::cerr << "\t-a,--arg\t\tArgument number\n";
			::std::cerr << "\t-f,--function\t\tFunction name\n";
			return c == 'h' ? 0 : 1;
		}
	}
	if (conf.param.empty() && (conf.arg == -1 || conf.function.empty())) {
		::std::cerr << "ERROR: No parameter name or argument provided!\n";
		return 2;
	}
	if (conf.lower_bound.size() != conf.upper_bound.size()) {
		::std::cerr << "ERROR: Mismatch in number of upper and lower bounds!\n";
		return 2;
	}

	static ::llvm::LLVMContext context;
	::std::deque<::std::pair<::std::unique_ptr<::llvm::Module>, ::std::string>> modules;
	for (int i = optind; i < argc; ++i) {
		::std::cout << "Parsing file: " << argv[i] << "\n";
		::llvm::SMDiagnostic error;
		modules.push_back(::std::make_pair(::llvm::parseIRFile(argv[i], error, context), argv[i]));
	}

	for (auto &m:modules) {
		for (auto &f:m.first->functions())
			apply_function_arg_assumption(f, conf);
		if (!conf.param.empty())
			apply_module_param_assumption(m.first.get(), conf);
		::std::error_code ec;
		::llvm::raw_fd_ostream os(m.second, ec, ::llvm::sys::fs::F_Text);
		m.first->print(os, nullptr);
		os.close();
	}

	return 0;
}
