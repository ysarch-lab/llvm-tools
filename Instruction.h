#pragma once
#include <ostream>
#include <string>

#include <llvm-c/Core.h>

class Instruction {
	LLVMValueRef inst_ = nullptr;
public:
	Instruction(LLVMValueRef &inst):inst_(inst) {}

	LLVMValueRef getMetadata(unsigned kind) const
	{ return LLVMGetMetadata(inst_, kind); }

	friend ::std::ostream & operator << (::std::ostream &, const Instruction &);
};
