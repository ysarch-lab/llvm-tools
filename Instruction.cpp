#include "Instruction.h"

::std::ostream & operator << (::std::ostream &O, const Instruction &i)
{
	O << i.getOpcode();
	return O;
}
