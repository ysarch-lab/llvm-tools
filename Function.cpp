#include "Function.h"
#include "Module.h"

#include <llvm-c/Core.h>

Module Function::getModule() const
{
	return Module(LLVMGetGlobalParent(func_));
}

::std::ostream & operator << (::std::ostream &O, const Function &f)
{
	O << f.getName();
	return O;
}
