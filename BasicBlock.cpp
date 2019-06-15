#include "BasicBlock.h"
#include "Function.h"

size_t ::std::hash<BasicBlock>::operator()(const BasicBlock &bb) const
{
	return (size_t)bb.bb_;
}

Function BasicBlock::getParent() const {
	return Function(LLVMGetBasicBlockParent(bb_));
}
