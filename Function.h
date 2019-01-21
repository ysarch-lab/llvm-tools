#pragma once
#include <memory>
#include <ostream>
#include <string>

#include <llvm-c/Core.h>
#include "BasicBlock.h"

class Function {
	LLVMValueRef func_ = nullptr;
public:
	Function(LLVMValueRef f):func_(f) {}

	::std::string getName() const
	{ return ::std::string(LLVMGetValueName(func_)); }

	class bb_iterator {
		LLVMValueRef &func_;
		LLVMBasicBlockRef bb_;
	public:
		bb_iterator(LLVMValueRef &f, LLVMBasicBlockRef bb):func_(f), bb_(bb) {}

		bool operator != (const bb_iterator &bb)
		{ return bb_ != bb.bb_; }

		BasicBlock operator *()
		{ return BasicBlock(bb_); }

		::std::unique_ptr<BasicBlock> operator -> ()
		{ return ::std::unique_ptr<BasicBlock>(new BasicBlock(bb_)); }

		const bb_iterator & operator++()
		{ bb_ = LLVMGetNextBasicBlock(bb_); return *this; }
	};

	bb_iterator begin()
	{ return bb_iterator(func_, LLVMGetFirstBasicBlock(func_)); }

	bb_iterator end()
	{ return bb_iterator(func_, nullptr); }

	friend ::std::ostream & operator << (::std::ostream &, const Function &);
};
