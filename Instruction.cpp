#include "Instruction.h"

::std::ostream & operator << (::std::ostream &O, const Instruction &i)
{
	auto op = LLVMGetInstructionOpcode(i.inst_);
	O << op;
	return O;
}
