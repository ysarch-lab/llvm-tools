#include "Instruction.h"

#include <llvm-c/Core.h>

::std::ostream & operator << (::std::ostream &O, const Instruction &i)
{
	char * str = LLVMPrintValueToString(i.inst_);
	O << str;
	LLVMDisposeMessage(str);
	return O;
}
::std::string Instruction::getName() const
{
	const char * name = LLVMGetValueName(inst_);
	::std::string ret;
	if (name == nullptr) {
		char * val = LLVMPrintValueToString(inst_);
		ret.assign(val);
		ret.resize(ret.find(','));
		LLVMDisposeMessage(val);
	} else {
		ret.assign(name);
	}
	return ret;
}
