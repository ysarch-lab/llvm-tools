#pragma once
#include <ostream>
#include <string>

#include <llvm-c/Core.h>

#include "Instruction.h"

class BasicBlock {
	LLVMBasicBlockRef bb_ = nullptr;
public:
	BasicBlock(LLVMBasicBlockRef &bb):bb_(bb) {}

	class inst_iterator {
		LLVMBasicBlockRef &bb_;
		LLVMValueRef inst_;
	public:
		inst_iterator(LLVMBasicBlockRef &bb, LLVMValueRef inst):bb_(bb), inst_(inst) {}

		bool operator != (const inst_iterator &i)
		{ return inst_ != i.inst_; }

		Instruction operator *()
		{ return Instruction(inst_); }

		::std::unique_ptr<Instruction> operator -> ()
		{ return ::std::unique_ptr<Instruction>(new Instruction(inst_)); }

		const inst_iterator & operator++()
		{ inst_ = LLVMGetNextInstruction(inst_); return *this; }
	};

	inst_iterator begin()
	{ return inst_iterator(bb_, LLVMGetFirstInstruction(bb_)); }

	inst_iterator end()
	{ return inst_iterator(bb_, nullptr); }

	friend ::std::ostream & operator << (::std::ostream &, const BasicBlock &);
};
