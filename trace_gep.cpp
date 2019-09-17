#include "trace_gep.h"

#include <iostream>

::llvm::Instruction *argument_to_caller_arg(::llvm::Argument *arg)
{
	auto fn = arg->getParent();
	// Check that we have only one call size
	assert(fn->hasOneUse());

	::llvm::User *user = *(fn->user_begin());
	::llvm::CallInst *call_site = ::llvm::cast<::llvm::CallInst>(user);
	unsigned op_num = arg->getArgNo();
	::llvm::Value *operand = call_site->getOperand(op_num);

	// Return operand instruction
	if (::llvm::isa<::llvm::Instruction>(operand))
		return ::llvm::cast<::llvm::Instruction>(operand);

	// Recurse if operand is another argument
	return argument_to_caller_arg(::llvm::cast<::llvm::Argument>(operand));
}

int_seq trace_gep(::llvm::Instruction *inst, ::llvm::AllocaInst* &alloca_i) {
	if (::llvm::AllocaInst *i = ::llvm::dyn_cast<::llvm::AllocaInst>(inst)) {
		assert(i->getName() == "const_params_loc");
		alloca_i = i;
		return int_seq({::llvm::ConstantInt::get(i->getContext(), ::llvm::APInt(32, 0))});
	}
	::llvm::GetElementPtrInst *i = ::llvm::cast<::llvm::GetElementPtrInst>(inst);

	int_seq indices;
	::llvm::Value *ptr = i->getPointerOperand();
	if (::llvm::Instruction * ptr_i = ::llvm::dyn_cast<::llvm::Instruction>(ptr)) {
		indices = trace_gep(ptr_i, alloca_i);
	} else {
		::llvm::Argument *arg = ::llvm::cast<::llvm::Argument>(ptr);
		indices = trace_gep(argument_to_caller_arg(arg), alloca_i);
	}
	for (auto idx_i = i->idx_begin() + 1; idx_i != i->idx_end(); ++idx_i) {
		indices.push_back(::llvm::cast<::llvm::ConstantInt>(*idx_i));
	}

	return indices;
}

::std::pair<::llvm::Instruction *, int_seq>
find_src_and_indices(::llvm::Module *m, const ::std::string &name)
{
	for (auto &f:m->functions())
		for (auto bbi = f.begin(); bbi != f.end(); ++bbi)
			for (auto ii = bbi->begin(); ii != bbi->end(); ++ii) {
				::llvm::AllocaInst * alloca_i;
				if (ii->getName() == name) {
					::std::cerr << "Found: " << name
					            << " in " << f.getName().str()
						    << "\n";
					::llvm::Instruction *i = &*ii;
					if (::llvm::isa<::llvm::LoadInst>(*ii))
					// The first param is pointer
						i = ::llvm::cast<::llvm::Instruction>(ii->op_begin()->get());
					assert(::llvm::isa<::llvm::GetElementPtrInst>(i));
					auto seq = trace_gep(i, alloca_i);
					return ::std::make_pair(alloca_i, seq);
				}
			}
	return ::std::make_pair(nullptr, int_seq());
}
