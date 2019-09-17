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
	{"lower-bound", required_argument, NULL, 'l'},
	{"upper-bound", required_argument, NULL, 'u'},
	{"help", no_argument, NULL, 'h'},
};

struct config {
	::std::string param;
	::std::deque<double> lower_bound;
	::std::deque<double> upper_bound;
};

template<typename B, typename C>
static void insert_assume(B &builder, ::llvm::Value *val, C lower, C upper)
{
	// Get assume Intrinsic
	::llvm::Intrinsic::ID id =
	  ::llvm::Function::lookupIntrinsicID("llvm.assume");
	::llvm::Function *assume_decl =
	  ::llvm::Intrinsic::getDeclaration(
	    builder.GetInsertBlock()->getParent()->getParent(), id);

	if (lower == upper) {
		::llvm::Value *eq_cond = builder.CreateFCmpOEQ(val,
		    ::llvm::ConstantFP::get(val->getType(), upper), "eq_bound");
		builder.CreateCall(assume_decl->getFunctionType(), assume_decl, {eq_cond});
	} else {
		assert(lower < upper);

		::llvm::Value *upper_cond = builder.CreateFCmpOLT(val,
		    ::llvm::ConstantFP::get(val->getType(), upper), "upper_bound");
		::llvm::Value *lower_cond = builder.CreateFCmpOGE(val,
		    ::llvm::ConstantFP::get(val->getType(), lower), "lower_bound");

		builder.CreateCall(assume_decl->getFunctionType(), assume_decl, {upper_cond});
		builder.CreateCall(assume_decl->getFunctionType(), assume_decl, {lower_cond});
	}
}

static void apply_assumption(::llvm::Instruction *src, const int_seq &indices,
		             const ::std::deque<double> &lower,
			     const ::std::deque<double> upper)
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

	::llvm::Value *load_orig = builder.CreateLoad(orig_gep, "load_orig");
	::llvm::Value *store_orig = builder.CreateStore(load_orig, alloca_gep);

	::llvm::Value *param_val = load_orig;
	::llvm::Type *param_type = param_val->getType();

	// Create bound conditions
	if (param_type->isArrayTy()) {
		const int64_t num_elements = param_type->getArrayNumElements();
		assert(num_elements == upper.size());
		assert(num_elements == lower.size());
		for (unsigned i = 0; i < num_elements; ++i) {
			::llvm::Value *element_val =
			    builder.CreateExtractValue(param_val, {i}, "extract_param_val");
			insert_assume(builder, element_val, lower[i], upper[i]);
		}
	}
}

static void apply_module_param_assumption(::llvm::Module *m,
                                          const ::std::string &file,
				          const config &conf)
{
	auto res = find_src_and_indices(m, conf.param);
	if (!res.second.empty()) {
		apply_assumption(res.first, res.second, conf.lower_bound, conf.upper_bound);
		::std::error_code ec;
		::llvm::raw_fd_ostream os(file, ec, ::llvm::sys::fs::F_Text);
		m->print(os, nullptr);
		os.close();
	}
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
	char c = -1;
	config conf;
	while ((c = getopt_long(argc, argv, "sp:l:u:h", options, NULL)) != -1) {
		switch (c) {
		case 'p': conf.param = ::std::string(optarg); break;
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
			return c == 'h' ? 0 : 1;
		}
	}
	if (conf.param.empty()) {
		::std::cerr << "ERROR: No parameter name provided!\n";
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
		apply_module_param_assumption(m.first.get(), m.second, conf);
	}

	return 0;
}
