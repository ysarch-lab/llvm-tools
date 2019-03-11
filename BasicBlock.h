#pragma once
#include <memory>
#include <ostream>
#include <string>

#include <llvm-c/Core.h>

#include "Instruction.h"

class BasicBlock;

namespace std {
template <> struct hash<BasicBlock> {
	size_t operator()(const BasicBlock &bb) const;
};
};

class BasicBlock {
	LLVMBasicBlockRef bb_ = nullptr;
public:
	BasicBlock(LLVMBasicBlockRef &bb):bb_(bb) {}

	static BasicBlock fromIntruction(const Instruction &i);

	const ::std::string getName() const
	{ return ::std::string(LLVMGetBasicBlockName(bb_)); }

	bool operator == (const BasicBlock &other) const
	{ return bb_ == other.bb_; }

	class inst_iterator {
		const LLVMBasicBlockRef &bb_;
		LLVMValueRef inst_;
	public:
		inst_iterator(const LLVMBasicBlockRef &bb, LLVMValueRef inst):bb_(bb), inst_(inst) {}

		bool operator != (const inst_iterator &i)
		{ return inst_ != i.inst_; }

		Instruction operator *()
		{ return Instruction(inst_); }

		::std::unique_ptr<Instruction> operator -> ()
		{ return ::std::unique_ptr<Instruction>(new Instruction(inst_)); }

		const inst_iterator & operator++()
		{ inst_ = LLVMGetNextInstruction(inst_); return *this; }
	};

	inst_iterator begin() const
	{ return inst_iterator(bb_, LLVMGetFirstInstruction(bb_)); }

	inst_iterator end() const
	{ return inst_iterator(bb_, nullptr); }

	class successor_iterator {
		LLVMValueRef bb_term_;
		unsigned successor_id_;
		::std::unique_ptr<BasicBlock> instance_ = nullptr;
	public:
		successor_iterator(LLVMBasicBlockRef &bb, int successor = 0):bb_term_(LLVMGetBasicBlockTerminator(bb)), successor_id_(successor)
		{
			if (successor == -1)
				successor_id_ = LLVMGetNumSuccessors(bb_term_);
		}

		bool operator != (const successor_iterator &i)
		{ return successor_id_ != i.successor_id_; }

		const successor_iterator & operator++()
		{ ++successor_id_; instance_ = nullptr; return *this; }


		BasicBlock& operator *()
		{ return *get_instance(); }

		BasicBlock* operator -> ()
		{ return get_instance(); }

	private:
		BasicBlock *get_instance() {
			if (!instance_) {
				auto ref = LLVMGetSuccessor(bb_term_, successor_id_);
				instance_ = ::std::unique_ptr<BasicBlock>(new BasicBlock(ref));
			}
			return instance_.get();
		}
	};

	successor_iterator successor_begin()
	{ return successor_iterator(bb_); }

	successor_iterator successor_end()
	{ return successor_iterator(bb_, -1); }

	friend ::std::ostream & operator << (::std::ostream &, const BasicBlock &);
	friend size_t ::std::hash<BasicBlock>::operator() (const BasicBlock &) const;
};
