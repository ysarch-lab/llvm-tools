#pragma once
#include <ostream>
#include <string>

#include <llvm-c/Core.h>

class Function {
	LLVMValueRef func_ = nullptr;
public:
	Function(LLVMValueRef f):func_(f) {}

	::std::string getName() const
	{ return ::std::string(LLVMGetValueName(func_)); }

	friend ::std::ostream & operator << (::std::ostream &, const Function &);
};
