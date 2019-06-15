#pragma once
#include <cassert>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

#include <llvm-c/Core.h>

class BasicBlock;

class Instruction {
	LLVMValueRef inst_ = nullptr;
public:
	Instruction() {}
	Instruction(LLVMValueRef &&inst):inst_(inst)
	{ assert(LLVMIsAInstruction(inst_)); }
	Instruction(LLVMValueRef &inst):inst_(inst)
	{ assert(LLVMIsAInstruction(inst_)); }

	LLVMValueRef getMetadata(unsigned kind) const
	{ return LLVMGetMetadata(inst_, kind); }

	LLVMValueRef getCalledValue() const {
		assert(getOpcode() == LLVMCall ||
		       getOpcode() == LLVMInvoke);
		return LLVMGetCalledValue(inst_);
	}

	::std::string getName() const;

	LLVMValueRef value() const
	{ return inst_; }

	BasicBlock getParent() const;

	LLVMOpcode getOpcode() const
	{ return LLVMGetInstructionOpcode(inst_); }

	int getNumOperands() const
	{ return LLVMGetNumOperands(inst_); }

	Instruction getOperandInstruction(int idx) const
	{ return Instruction(LLVMGetOperand(inst_, idx)); }

	class op_iterator {
		Instruction &inst_;
		int op_index_;
		::std::unique_ptr<Instruction> instance_ = nullptr;
	public:
		op_iterator(Instruction &i, int idx): inst_(i), op_index_(idx) {}

		bool operator != (const op_iterator &i)
		{ return &inst_ != &i.inst_ || op_index_ != i.op_index_; }

		Instruction& operator *()
		{ return *get_instance_(); }

		Instruction* operator -> ()
		{ return get_instance_(); }

		bool isInstruction()
		{ return !!get_instance_(); }

		const op_iterator & operator++()
		{ ++op_index_; instance_ = nullptr; return *this; }

		LLVMValueRef value() const
		{ return LLVMGetOperand(inst_.inst_, op_index_); }
	private:
		Instruction * get_instance_() {
			if (!instance_) {
				LLVMValueRef v = value();
				if (LLVMIsAInstruction(v))
					instance_.reset(new Instruction(v));
			}
			return instance_.get();
		}
	};

	op_iterator op_begin()
	{ return op_iterator(*this, 0); }

	op_iterator op_end()
	{ return op_iterator(*this, getNumOperands()); }

	class use_iterator {
		const Instruction &inst_;
		LLVMUseRef use_ = nullptr;
		::std::unique_ptr<Instruction> instance_ = nullptr;
	public:
		use_iterator(const Instruction &i, LLVMUseRef &&u): inst_(i), use_(u) {}

		bool operator != (const use_iterator &i)
		{ return &inst_ != &i.inst_ || use_ != i.use_; }

		Instruction& operator *()
		{ return *get_instance_(); }

		Instruction* operator -> ()
		{ return get_instance_(); }

		bool isInstruction()
		{ return !!get_instance_(); }

		const use_iterator & operator++()
		{ use_ = LLVMGetNextUse(use_); instance_ = nullptr; return *this; }

		LLVMValueRef value() const
		{ return LLVMGetUser(use_); }
	private:
		Instruction * get_instance_() {
			if (!instance_) {
				LLVMValueRef v = value();
				if (LLVMIsAInstruction(v))
					instance_.reset(new Instruction(v));
			}
			return instance_.get();
		}
	};

	use_iterator use_begin() const
	{ return use_iterator(*this, LLVMGetFirstUse(inst_)); }

	use_iterator use_end() const
	{ return use_iterator(*this, nullptr); }

	friend class BasicBlock;
	friend class op_iterator;
	friend class use_iterator;
	friend ::std::ostream & operator << (::std::ostream &, const Instruction &);
};

::std::ostream & operator << (::std::ostream &, const Instruction &);
