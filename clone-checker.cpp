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


#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/LazyValueInfo.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/DebugLoc.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Transforms/Utils/FunctionComparator.h>
#include"debug_location.h"

static const struct option options[] = {
	{"functionF", required_argument, NULL, 'f'},
	{"functionG", required_argument, NULL, 'g'},
	{"help", no_argument, NULL, 'h'},
};

::llvm::GlobalNumberState GlobalNumbers;
::llvm::raw_os_ostream llvm_cout(::std::cout);
::llvm::raw_ostream &llvm::dbgs();

int compareBB(::llvm::Function *G, ::llvm::Function *F, const ::llvm::BasicBlock* BBL, const ::llvm::BasicBlock *BBR)
{
	int flag = 0;
	::llvm::BasicBlock::const_iterator InstL = BBL->begin(), InstLE = BBL->end();
	::llvm::BasicBlock::const_iterator InstR = BBR->begin(), InstRE = BBR->end();
	
	///static const unsigned dbg_kind_id = LLVMGetMDKindID("dbg", 3);
	do {
		::llvm::MDNode *MD = InstL->getMetadata("DISubprogramKind");
		if(InstL->getOpcode() == InstR->getOpcode() && InstL->getNumOperands() == InstR->getNumOperands() ) {
			for(int i = 0; i < InstL->getNumOperands(); i++)
			{
				if(InstL->getOperand(i) != InstR->getOperand(i))
				{
					flag = 1;
					break;
				}
			}
		}
		else {
			flag = 1;
		}
			//::std::cout << "might be same";
		if(flag != 0)
		{
			//Ignore gep
			if(InstR->getOpcode() != 34 && InstL->getOpcode() != 34){
		       ::std::string str1; ::llvm::raw_os_ostream errs1(::std::cerr);
                        ::llvm::raw_string_ostream ss1(str1);
                        ss1 << *InstL;                
                        errs1 << "\n\n\n\nLHS Instruction :" << ss1.str() << "\n";

			::std::string str; ::llvm::raw_os_ostream errs(::std::cerr);
			::llvm::raw_string_ostream ss(str);
			ss << *InstR;
			errs << "\n\n\n\nRHS Instruction: "<< ss.str() << "\n";
			const ::llvm::DebugLoc dbgLoc3 = InstL->getDebugLoc();
        		const ::llvm::MDNode *Scope = dbgLoc3.getScope();
        		::llvm::DILocation *DIloc = dbgLoc3.get();
			::llvm::DILocation *InlinedAt = DIloc->getInlinedAt();
			::std::string currLFunc = DIloc->getScope()->getSubprogram()->getName();
			::std::deque<::std::string> Lqueue;
			Lqueue.push_front(currLFunc);
			while(InlinedAt != NULL) 
			{ 
				Lqueue.push_front(InlinedAt->getScope()->getSubprogram()->getName());
				InlinedAt = InlinedAt->getInlinedAt();
			}

			errs1 << "\nLHS: Inlined ladder: ";//DIloc->getScope()->getSubprogram()->getName();

			for(int i = 0; i < Lqueue.size(); i++)
			{
				
				errs1 << Lqueue.at(i) << "\t";
			}


			//::std::cout<<"\n\nDEBUG INFO: INST RHS";
        		const ::llvm::DebugLoc dbgLoc2 = InstR->getDebugLoc();
        		//const ::llvm::DebugLoc dbgLoc3 = InstL->getDebugLoc();
        		const ::llvm::MDNode *Scope2 = dbgLoc2.getScope();
        		::llvm::DILocation *DIloc2 = dbgLoc2.get();
        		//llvm_cout 
			::llvm::DILocation *RInlinedAt = DIloc2->getInlinedAt();
			::std::deque<::std::string> Rqueue;
			::std::string Rstr = DIloc2->getScope()->getSubprogram()->getName();
			Rqueue.push_front(Rstr);
			while(RInlinedAt != NULL)
			{
				Rqueue.push_front(RInlinedAt->getScope()->getSubprogram()->getName());
				RInlinedAt = RInlinedAt -> getInlinedAt(); 
			}
			errs << "\nRHS: Inlined ladder: ";//<<DIloc2->getScope()->getSubprogram()->getName();
			for(int i = 0; i < Rqueue.size(); i++)
			{
				errs << Rqueue.at(i) << "\t";
			}

			//dbgLoc.dump();
        		//dbgLoc2.print(llvm::dbgs());
		//	::std::string dir = dbgLoc2.getDirectory();
		//	::std::string file = dbgLoc2.getFileName();
		        //int line = dbgLoc2.getLine();
			//int column = dbgLoc2.getCol();

			//::std::cout<<"\n\ndir: "<< dir << "\nfile"<< file<<"\nline" << line << "\ncol" << column;

			}	
		}
/*		::std::cout<<"Comparing  OpName: "<<InstL->getOpcodeName()<<" and  "<<InstR->getOpcodeName() << "res: "<< res << "\n";
		if(res == false) {
			::std::cout << "DIFFERENT";
			::std::cout<<"InstL op: " << InstL->getOpcodeName()<<"InstR op: "<< InstR->getOpcodeName();
			return 0;
		}*/
		++InstL;
		++InstR;
		//res = InstL->isIdenticalTo(&*InstR);
		//::std::cout<< "\nOpcode: " << InstL.getOpcode() << "   OpName: " << InstL.getOpcodeName();
	}while(InstL != InstLE && InstR != InstRE);
	
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


int main(int argc, char **argv) {
	char c = -1;
	int cmpRes1 = 9999, cmpRes2 = 9999;
	config conf1, conf2;
	::llvm::LLVMContext context, context2;
	::llvm::Function *G; ::llvm::Function *F;

	while ((c = getopt_long(argc, argv, "f:g:h", options, NULL)) != -1) {
		switch (c) {
		case 'f': conf1.func = ::std::string(optarg); break;
		case 'g': conf2.func = ::std::string(optarg); break;
		default:
			::std::cerr << "Unknown option: " << argv[optind - 1]
			            << ::std::endl;
		case 'h':
			::std::cerr << "Available options:\n";
			::std::cerr << "\t\t-f,--function <f> Function to examine from module1.\n";
			::std::cerr << "\t\t-g, --function <g> Function to examine from module2.";
			return c == 'h' ? 0 : 1;
		}
	}
	
	/*if(conf1.func != conf2.func)
	{
		::std::cout << "Will not compare different functions for equivalence.";
		return 0;
	}*/
	//static ::llvm::LLVMContext context, context2;
	::std::deque<::std::unique_ptr<::llvm::Module>> modules, modules2;
	int i = optind;
	while (i < argc) {
		::std::cout << "Parsing file1: " << argv[i] << "\n";
		::std::cout << "Parsing file2: " << argv[i + 1] << "\n";
		if(::std::string(argv[i]) == ::std::string(argv[i + 1]))
		{
			::std::cout << "\nEquivalent functions\n";
			return 0;
		}
		::llvm::raw_os_ostream errs(::std::cerr);
		::llvm::SMDiagnostic error;
		auto ptr = ::llvm::parseIRFile(argv[i], error, context);
		auto ptr2 = ::llvm::parseIRFile(argv[++i], error, context2);
		if (ptr && ptr2) {
			modules.push_back(::std::move(ptr));
			modules2.push_back(::std::move(ptr2));
		} else {
			error.print(argv[i-1], errs);
			error.print(argv[i],errs);
		}
		++i;
	}


	for(auto &m:modules) {
		for(auto &f:m->functions())
		{
			if(conf1.func == f.getName()) {
				F = m->getFunction(f.getName());
				break;
			}
			//llvm_cout << "f.getName() " << f.getName() <<"\n";
		}
	}
	
	for(auto &n:modules2) {
		for(auto &g:n->functions())
		{	
			if(conf2.func == g.getName()) {
				G = n->getFunction(g.getName());
				break;
			}
			//llvm_cout << "g.getName() "<< g.getName()<< "\n";
		}
	}

	::std::cout<<"\nComparing: " << conf1.func << " " << conf2.func;
	cmpRes1 = ::llvm::FunctionComparator(F, G, &GlobalNumbers).compare();
        cmpRes2 = ::llvm::FunctionComparator(G, F, &GlobalNumbers).compare();
	
	//llvm_cout << "\ncmpres1:" << cmpRes1 << "\n" << "cmpres2: " << cmpRes2;
	if(cmpRes1 == 0 && cmpRes2 == 0)
	{
		::std::cout << "\nEquivalent functions\n";
	}
	::llvm::SmallVector<const ::llvm::BasicBlock*, 8> FnLBBs, FnRBBs;
	// Traverse in CFG ordering. maybe improve further later
	FnLBBs.push_back(&F->getEntryBlock());
	FnRBBs.push_back(&G->getEntryBlock());
	while(!FnLBBs.empty())
	{
		const ::llvm::BasicBlock *BBL = FnLBBs.pop_back_val();
		const ::llvm::BasicBlock *BBR = FnRBBs.pop_back_val();
		compareBB(G, F, BBL, BBR);
	}
	return 0;
}
