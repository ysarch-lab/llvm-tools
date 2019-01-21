#pragma once
#include <ostream>
#include <string>

#include <llvm-c/Core.h>

class BasicBlock {
	LLVMBasicBlockRef bb_ = nullptr;
public:
	BasicBlock(LLVMBasicBlockRef bb):bb_(bb) {}

	friend ::std::ostream & operator << (::std::ostream &, const BasicBlock &);
};
