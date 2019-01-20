#include <ostream>

#include <llvm-c/Core.h>

class Function {
	LLVMValueRef func_ = nullptr;
public:
	Function(LLVMValueRef f):func_(f) {}

	friend ::std::ostream & operator << (::std::ostream &, const Function &);
};
