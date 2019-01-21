#include "Function.h"

::std::ostream & operator << (::std::ostream &O, const Function &f)
{
	O << f.getName();
	return O;
}
