#include "BasicBlock.h"

size_t ::std::hash<BasicBlock>::operator()(const BasicBlock &bb) const
{
	return (size_t)bb.bb_;
}

BasicBlock BasicBlock::fromIntruction(const Instruction &i)
{
	LLVMBasicBlockRef bb = LLVMGetInstructionParent(i.inst_);
	return BasicBlock(bb);
}
