#include "Function.h"

::std::ostream & operator << (::std::ostream &O, const Function &f)
{
	O << LLVMGetValueName(f.func_);
	return O;
}
