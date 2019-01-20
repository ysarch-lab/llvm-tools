#include <ostream>

#include <llvm-c/Core.h>


class Module {
	LLVMModuleRef module_ = nullptr;
public:
	Module(const char* file);
	~Module();

	friend ::std::ostream & operator << (::std::ostream &, const Module &);
};
::std::ostream & operator << (::std::ostream &O, const Module &m);
