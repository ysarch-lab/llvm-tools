#include "BasicBlock.h"

size_t ::std::hash<BasicBlock>::operator()(const BasicBlock &bb) const
{
	return (size_t)bb.bb_;
}
