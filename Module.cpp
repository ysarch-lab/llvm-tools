#include <iostream>
#include <stdexcept>

#include <llvm-c/Analysis.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/BitReader.h>
#include <llvm-c/Core.h>

#include "Module.h"

Module::Module(const char *file)
{
	LLVMMemoryBufferRef file_buf = nullptr;
	char *msg = nullptr;
	if (LLVMCreateMemoryBufferWithContentsOfFile(file, &file_buf, &msg))
		throw ::std::runtime_error(msg);

	msg = nullptr;
	if (LLVMParseIRInContext(LLVMGetGlobalContext(), file_buf, &module_, &msg))
		throw ::std::runtime_error(msg);
}

Module::~Module()
{
	if (created_)
		LLVMDisposeModule(module_);
}

::std::string Module::getName() const
{
	size_t size = 0;
	const char *name = LLVMGetModuleIdentifier(module_, &size);
	return ::std::string(name, size);
}

::std::ostream & operator << (::std::ostream &O, const Module &m)
{
	char * mod = LLVMPrintModuleToString(m.module_);
	O << mod;
	return O;
}
