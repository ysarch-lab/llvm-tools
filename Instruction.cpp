#include "Instruction.h"

::std::ostream & operator << (::std::ostream &O, const Instruction &i)
{
	char * str = LLVMPrintValueToString(i.inst_);
	O << str;
	LLVMDisposeMessage(str);
	return O;
}
