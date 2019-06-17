#include <getopt.h>

#include <cassert>
#include <deque>
#include <iostream>
#include <string>

#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/LazyValueInfo.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/SourceMgr.h>

static const struct option options[] = {
	{"function", required_argument, NULL, 'f'},
	{"arg", required_argument, NULL, 'a'},
	{"help", no_argument, NULL, 'h'},
};

using val_list = ::std::deque<::std::pair<::llvm::Value*, ::llvm::Instruction*>>;

static void get_stored_values(::llvm::Value *val, val_list &stored_vals) {
	if (::llvm::StoreInst *i = ::llvm::dyn_cast<::llvm::StoreInst>(val)) {
		::std::cerr << "Used in store: ";
		i->dump();
		stored_vals.push_back(::std::make_pair(i->getValueOperand(), i));

	}
	for (auto u:val->users())
		get_stored_values(u, stored_vals);
}

static val_list find_arg_values(::llvm::Function &f, unsigned arg) {
	// Run analysis pass on function
	::std::cerr << "Running on function: " << f.getName().str() << "\n";
	::llvm::Argument &output_arg = f.arg_begin()[arg];
	::std::cerr << "Output argument: ";
	output_arg.dump();
	val_list vals;
	get_stored_values(&output_arg, vals);
	return vals;
}

struct config {
	::std::string func = "run_";
	unsigned arg = 4;
};

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
			return c == 'h' ? 0 : 1;
		}
	}

	static ::llvm::LLVMContext context;
	::std::deque<::std::unique_ptr<::llvm::Module>> modules;
	for (int i = optind; i < argc; ++i) {
		::std::cout << "Parsing file: " << argv[i] << "\n";
		::llvm::SMDiagnostic error;
		modules.push_back(::llvm::parseIRFile(argv[i], error, context));
	}

	for (auto &m:modules) {
		for (auto &f: m->functions()) {
			auto mm = ::std::mismatch(conf.func.begin(),
			                          conf.func.end(),
						  f.getName().begin());
			if (mm.first == conf.func.end()) {
				::llvm::legacy::PassManager pm;
				::llvm::Pass *p = ::llvm::createLazyValueInfoPass();
				pm.add(p);
				pm.run(*m);
				::llvm::LazyValueInfoWrapperPass *wp =
					(::llvm::LazyValueInfoWrapperPass *)p;
				auto &lvi = wp->getLVI();
				for (auto v:find_arg_values(f, conf.arg)) {
					lvi.getConstantRange(v.first, v.second->getParent()).dump();
					::std::cerr << " is known about: ";
					v.first->dump();
				}
				wp->releaseMemory();
			}
		}
	}

	return 0;
}
