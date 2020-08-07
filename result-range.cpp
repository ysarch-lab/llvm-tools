#include "trace_gep.h"

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
#include <llvm/Support/raw_os_ostream.h>

static const struct option options[] = {
	{"function", required_argument, NULL, 'f'},
	{"arg", required_argument, NULL, 'a'},
	{"no-signed-zeros", no_argument, NULL, 'z'},
	{"verbose", no_argument, NULL, 'v'},
	{"help", no_argument, NULL, 'h'},
};

using val_list = ::std::deque<::llvm::StoreInst*>;

::llvm::raw_os_ostream llvm_cout(::std::cout);

static void set_nsz(::llvm::Function &f)
{
	for (auto &bb: f)
		for (auto &i: bb)
			if (::llvm::isa<::llvm::FPMathOperator>(i))
				i.setHasNoSignedZeros(true);
}

static void get_stored_values(::llvm::Value *val, val_list &stored_vals) {
	if (::llvm::StoreInst *i = ::llvm::dyn_cast<::llvm::StoreInst>(val)) {
		llvm_cout << "Used in store:" << *i << "\n";
		stored_vals.push_front(i);
	}
	for (auto u:val->users())
		get_stored_values(u, stored_vals);
}

static val_list find_arg_values(::llvm::Function &f, unsigned arg) {
	// Run analysis pass on function
	llvm_cout << "Running on function: " << f.getName().str()
	           << " (" << f.arg_size() << " args)\n";
	if (arg >= f.arg_size()) {
		::std::cerr << "Argument " << arg << " not available. Function has only " << f.arg_size() << " arguments.\n";
		return val_list();
	}
	::llvm::Argument &output_arg = f.arg_begin()[arg];
	llvm_cout << "Argument " << arg << ": " << output_arg << "\n";
	val_list vals;
	get_stored_values(&output_arg, vals);
	if (vals.empty())
		llvm_cout << "No stores to selected location.\n";
	llvm_cout.flush();
	return vals;
}

struct config {
	::std::string func = "run_";
	unsigned arg = 4;
	bool nsz = false;
	bool verbose = false;
};

static void analyze_function(::llvm::Function &f, const config &conf)
{
	if (conf.nsz)
		set_nsz(f);
	::llvm::legacy::FunctionPassManager pm(f.getParent());
	::llvm::Pass *p = ::llvm::createLazyValueInfoPass();
	pm.add(p);
	pm.run(f);
	::llvm::LazyValueInfoWrapperPass *wp =
		(::llvm::LazyValueInfoWrapperPass *)p;
	auto &lvi = wp->getLVI();
	for (auto v:find_arg_values(f, conf.arg)) {
		// Pointer is the second arg for stores
		::llvm::Value *ptr = v->getPointerOperand();
		::llvm::GetElementPtrInst *gep =
			::llvm::dyn_cast<::llvm::GetElementPtrInst>(ptr);
		::llvm::AllocaInst *i;
		auto idx_seq = trace_gep(gep, i);
		for (const auto idx:idx_seq)
			llvm_cout << idx->getValue() << " ";
		llvm_cout << "is known to be in: "
			  << lvi.getConstantRange(v->getValueOperand(), v->getParent());
		if (conf.verbose)
			llvm_cout << " " << *v->getValueOperand();
		llvm_cout << "\n";
	}
	wp->releaseMemory();
}

int main(int argc, char **argv) {
	char c = -1;
	config conf;
	while ((c = getopt_long(argc, argv, "f:a:zvh", options, NULL)) != -1) {
		switch (c) {
		case 'f': conf.func = ::std::string(optarg); break;
		case 'a': conf.arg = ::std::stoi(optarg); break;
		case 'z': conf.nsz = true; break;
		case 'v': conf.verbose = true; break;
		default:
			::std::cerr << "Unknown option: " << argv[optind - 1]
			            << ::std::endl;
		case 'h':
			::std::cerr << "Available options:\n";
			::std::cerr << "\t\t-f,--function <f> Function to examine.\n";
			::std::cerr << "\t\t-a,--arg <a> Function output argument.\n";
			::std::cerr << "\t\t-z,--no-signed-zeros FP operations should ignore sign of 0.0.\n";
			::std::cerr << "\t\t-v,--verbose Print the insturction the produced the stored value\n";
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
