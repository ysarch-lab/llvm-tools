#pragma once
#include <cassert>
#include <ostream>
#include <string>

#include <llvm-c/Core.h>

class Instruction {
	LLVMValueRef inst_ = nullptr;
public:
	Instruction(LLVMValueRef &inst):inst_(inst) {}

	LLVMValueRef getMetadata(unsigned kind) const
	{ return LLVMGetMetadata(inst_, kind); }

	LLVMValueRef getCalledValue() const {
		assert(getOpcode() == LLVMCall ||
		       getOpcode() == LLVMInvoke);
		return LLVMGetCalledValue(inst_);
	}

	LLVMOpcode getOpcode() const
	{ return LLVMGetInstructionOpcode(inst_); }

	friend class BasicBlock;
};

::std::ostream & operator << (::std::ostream &, const Instruction &);
