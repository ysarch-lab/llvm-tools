#include <deque>
#include <utility>

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

using int_seq = ::std::deque<::llvm::ConstantInt *>;

::llvm::Instruction *argument_to_caller_arg(::llvm::Argument *arg);

int_seq trace_gep(::llvm::Instruction *inst, ::llvm::AllocaInst * &alloca_i);

::std::pair<::llvm::Instruction *, int_seq>
find_src_and_indices(::llvm::Module *m, const ::std::string &name);
