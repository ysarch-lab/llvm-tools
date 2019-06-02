#include "Instruction.h"

::std::ostream & operator << (::std::ostream &O, const Instruction &i)
{
	char * str = LLVMPrintValueToString(i.inst_);
	O << str;
	LLVMDisposeMessage(str);
	return O;
}
::std::string Instruction::getName() const
{
	size_t len = 0;
	const char * name = LLVMGetValueName2(inst_, &len);
	::std::string ret;
	if (len == 0 || name == nullptr) {
		char * val = LLVMPrintValueToString(inst_);
		ret.assign(val);
		ret.resize(ret.find(','));
		LLVMDisposeMessage(val);
	} else {
		ret.assign(name, len);
	}
	return ret;
}
