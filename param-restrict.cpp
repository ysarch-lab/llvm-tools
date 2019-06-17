#include <getopt.h>

#include <cassert>
#include <deque>
#include <iostream>
#include <string>
#include <vector>

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>

static const struct option options[] = {
	{"param", required_argument, NULL, 'p'},
	{"lower-bound", required_argument, NULL, 'l'},
	{"upper-bound", required_argument, NULL, 'u'},
	{"help", no_argument, NULL, 'h'},
};

struct config {
	::std::string param;
	double lower_bound = 0.0;
	double upper_bound = 0.0;
};

using int_seq = ::std::deque<::llvm::ConstantInt *>;

static int_seq trace_gep(::llvm::Instruction *inst, ::llvm::AllocaInst* &alloca_i) {
	if (::llvm::AllocaInst *i = ::llvm::dyn_cast<::llvm::AllocaInst>(inst)) {
		assert(i->getName() == "const_params_loc");
		alloca_i = i;
		return int_seq({::llvm::ConstantInt::get(i->getContext(), ::llvm::APInt(32, 0))});
	}
	::llvm::GetElementPtrInst *i = ::llvm::cast<::llvm::GetElementPtrInst>(inst);

	int_seq indices;
	::llvm::Value * ptr = i->getPointerOperand();
	if (::llvm::Instruction * ptr_i = ::llvm::dyn_cast<::llvm::Instruction>(ptr)) {
		indices = trace_gep(ptr_i, alloca_i);
	} else {
		::llvm::Argument * arg = ::llvm::cast<::llvm::Argument>(ptr);
		auto fn = arg->getParent();
		// Check that we have only one call size
		assert(fn->hasOneUse());
		::llvm::User * user = *(fn->user_begin());

		::llvm::CallInst * call_site = ::llvm::cast<::llvm::CallInst>(user);

		// Function to call is one of the arguments
		switch (call_site->getNumArgOperands()) {
		case 4: { // Component (Mechanism), params is the first arg
			::llvm::Instruction *p_op =
			  ::llvm::cast<::llvm::Instruction>(call_site->getOperand(0));
			indices = trace_gep(p_op, alloca_i);
			break;
		}
		case 5: { // Wrapper, params is the second arg
			::llvm::Instruction *p_op =
			  ::llvm::cast<::llvm::Instruction>(call_site->getOperand(1));
			indices = trace_gep(p_op, alloca_i);
			break;
		}
		default:
			assert(false);
		}
	}
	for (auto idx_i = i->idx_begin() + 1; idx_i != i->idx_end(); ++idx_i) {
		indices.push_back(::llvm::cast<::llvm::ConstantInt>(*idx_i));
	}

	return indices;
}

static ::std::pair<::llvm::Instruction *, int_seq>
find_src_and_indices(::llvm::Module *m, const ::std::string &name)
{
	for (auto &f:m->functions())
		for (auto bbi = f.begin(); bbi != f.end(); ++bbi)
			for (auto ii = bbi->begin(); ii != bbi->end(); ++ii) {
				::llvm::AllocaInst * alloca_i;
				if (ii->getName() == name) {
					::std::cerr << "Found: " << name
					            << " in " << f.getName().str()
						    << "\n";
					assert(::llvm::isa<::llvm::LoadInst>(*ii));
					// The first param is pointer
					::llvm::Instruction *i = ::llvm::cast<::llvm::Instruction>(ii->op_begin()->get());
					auto seq = trace_gep(i, alloca_i);
					return ::std::make_pair(alloca_i, seq);
				}
			}
	return ::std::make_pair(nullptr, int_seq());
}

static void apply_assumption(::llvm::Instruction *src, const int_seq &indices, double lower, double upper)
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
	::llvm::IRBuilder<> builder(store_val);
	::llvm::Value *orig_gep =
	  builder.CreateInBoundsGEP(param_arg, llvm_indices, "orig_gep");
	::llvm::Value *alloca_gep =
	  builder.CreateInBoundsGEP(src, llvm_indices, "dst_gep");

	::llvm::Value *load_orig = builder.CreateLoad(orig_gep, "load_orig");
	::llvm::Value *store_orig = builder.CreateStore(load_orig, alloca_gep);

	::llvm::Value *param_val = load_orig;
	if (param_val->getType()->isArrayTy())
		param_val = builder.CreateExtractValue(param_val, {0}, "extract_param_val");

	// Create bound conditions
	::llvm::Value * upper_cond = builder.CreateFCmpOLT(param_val,
	  ::llvm::ConstantFP::get(param_val->getType(), upper), "upper_bound");
	::llvm::Value * lower_cond = builder.CreateFCmpOGE(param_val,
	  ::llvm::ConstantFP::get(param_val->getType(), lower), "lower_bound");

	::llvm::Intrinsic::ID id =
	  ::llvm::Function::lookupIntrinsicID("llvm.assume");

	::llvm::Function *assume_decl =
	  ::llvm::Intrinsic::getDeclaration(f->getParent(), id);


	builder.CreateCall(assume_decl->getFunctionType(), assume_decl, {upper_cond});
	builder.CreateCall(assume_decl->getFunctionType(), assume_decl, {lower_cond});
}

int main(int argc, char **argv) {
	char c = -1;
	config conf;
	while ((c = getopt_long(argc, argv, "sp:l:u:h", options, NULL)) != -1) {
		switch (c) {
		case 'p': conf.param = ::std::string(optarg); break;
		case 'l': conf.lower_bound = ::std::stod(optarg); break;
		case 'u': conf.upper_bound = ::std::stod(optarg); break;
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

	static ::llvm::LLVMContext context;
	::std::deque<::std::pair<::std::unique_ptr<::llvm::Module>, ::std::string>> modules;
	for (int i = optind; i < argc; ++i) {
		::std::cout << "Parsing file: " << argv[i] << "\n";
		::llvm::SMDiagnostic error;
		modules.push_back(::std::make_pair(::llvm::parseIRFile(argv[i], error, context), argv[i]));
	}

	for (auto &m:modules) {
		auto res = find_src_and_indices(m.first.get(), conf.param);
		int_seq &indices = res.second;
		if (!res.second.empty()) {
			apply_assumption(res.first, res.second, conf.lower_bound, conf.upper_bound);
			::std::error_code ec;
			::llvm::raw_fd_ostream os(m.second, ec, ::llvm::sys::fs::F_Text);
			m.first->print(os, nullptr);
			os.close();
		}
	}

	return 0;
}
