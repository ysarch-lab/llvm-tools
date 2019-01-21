#pragma once
#include <memory>
#include <ostream>

#include <llvm-c/Core.h>

#include "Function.h"

class Module {
	LLVMModuleRef module_ = nullptr;
public:
	Module(const char* file);
	Module(Module &&m)
	{ ::std::swap(module_, m.module_); }
	~Module();

	LLVMModuleRef & get()
	{ return module_; }

	class func_iterator {
		Module &m_;
		LLVMValueRef func_;
	public:
		func_iterator(Module &m, LLVMValueRef f):m_(m), func_(f) {}

		bool operator != (const func_iterator &fi)
		{ return func_ != fi.func_; }

		Function operator *()
		{ return Function(func_); }

		::std::unique_ptr<Function> operator -> ()
		{ return ::std::unique_ptr<Function>(new Function(func_)); }

		func_iterator operator++()
		{ func_ = LLVMGetNextFunction(func_); }
	};

	func_iterator func_begin()
	{ return func_iterator(*this, LLVMGetFirstFunction(module_)); }

	func_iterator func_end()
	{ return func_iterator(*this, nullptr); }

	friend ::std::ostream & operator << (::std::ostream &, const Module &);
};
::std::ostream & operator << (::std::ostream &O, const Module &m);
