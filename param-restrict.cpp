#include <getopt.h>

#include <cassert>
#include <deque>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Module.h"
#include "debug_location.h"

static const struct option options[] = {
	{"param", required_argument, NULL, 'p'},
	{"lower-bound", required_argument, NULL, 'l'},
	{"upper-bound", required_argument, NULL, 'u'},
	{"help", no_argument, NULL, 'h'},
};

struct config {
	::std::string param;
	double lower_bound = 0.0;
	double upper_bound = 0.0;
};

using int_seq = ::std::deque<int>;

static int_seq trace_gep(Instruction &inst, Instruction &alloca_i) {
	if (inst.getOpcode() == LLVMAlloca) {
		assert(inst.getName() == "const_params_loc");
		alloca_i = inst;
		return int_seq({0});
	}
	assert(inst.getOpcode() == LLVMGetElementPtr);

	int_seq indices;
	auto opi = inst.op_begin();
	if (opi.isInstruction()) {
		indices = trace_gep(*opi, alloca_i);
	} else {
		assert(LLVMIsAArgument(opi.value()));
		auto fn = LLVMGetParamParent(opi.value());
		auto use = LLVMGetFirstUse(fn);
		// Check that we have only one call size
		assert(use != nullptr);
		assert(LLVMGetNextUse(use) == nullptr);

		Instruction call_site(LLVMGetUser(use));
		assert(call_site.getOpcode() == LLVMCall);

		// Function to call is one of the arguments
		switch (call_site.getNumOperands()) {
		case 5: { // Component (Mechanism)
			Instruction p_op = call_site.getOperandInstruction(0);
			indices = trace_gep(p_op, alloca_i);
			break;
		}
		case 6: { // Wrapper
			Instruction p_op = call_site.getOperandInstruction(1);
			indices = trace_gep(p_op, alloca_i);
			break;
		}
		default:
			assert(false);
		}
	}
	++opi; // Skip the first '0' index
	for (++opi; opi != inst.op_end(); ++opi) {
		auto val = opi.value();
		assert(LLVMIsConstant(val));
		long long i = LLVMConstIntGetZExtValue(val);
		indices.push_back(i);
	}

	return indices;
}

static ::std::pair<Instruction, int_seq> find_src_and_indices(Module &m, const ::std::string &name)
{
	for (auto it = m.func_begin(); it != m.func_end(); ++it)
		for (auto bbi = it->begin(); bbi != it->end(); ++bbi)
			for (auto ii = bbi->begin(); ii != bbi->end(); ++ii) {
				Instruction &i = *ii;
				Instruction alloca_i;
				if (i.getName() == name) {
					::std::cerr << "Found: " << name
					            << " in " << it->getName()
						    << "\n";
					assert(i.getOpcode() == LLVMLoad);
					// The first param is pointer
					auto seq = trace_gep(*i.op_begin(), alloca_i);
					return ::std::make_pair(alloca_i, seq);
				}
			}
	return ::std::make_pair(Instruction(), int_seq());
}

static void apply_assumption(const Instruction &src, const int_seq &indices, double lower, double upper)
{
	::std::cerr << "Source: " << src.getName() << "\n";
	::std::cerr << "Found indices:";
	for (int idx:indices)
		::std::cerr << " " << idx << ",";
	::std::cerr << "\n";

	LLVMValueRef store_val = nullptr;
	for (auto ui = src.use_begin(); ui != src.use_end(); ++ui) {
		if (ui->getOpcode() == LLVMStore) {
			assert(store_val == nullptr);
			store_val = ui.value();
		}
	}
	assert(store_val != nullptr);
	// Create builder and position after parameters set
	LLVMBuilderRef builder = LLVMCreateBuilder();
	LLVMPositionBuilder(builder, LLVMGetInstructionParent(store_val), store_val);

	std::vector<LLVMValueRef> llvm_indices;
	llvm_indices.reserve(indices.size()+1);
	for (int idx:indices) {
		llvm_indices.push_back(LLVMConstInt(LLVMInt32Type(), idx, false));
	}

	Function f = src.getParent().getParent();
	::std::cerr << f.getName() << "\n";
	LLVMValueRef param_arg = LLVMGetParam(f.value(), 1);
	LLVMValueRef orig_gep = LLVMBuildGEP(builder,
	                                     param_arg, llvm_indices.data(),
					     llvm_indices.size(), "orig_gep");
	LLVMValueRef alloca_gep = LLVMBuildGEP(builder,
	                                       src.value(), llvm_indices.data(),
					       llvm_indices.size(), "dst_gep");
	LLVMValueRef load_orig = LLVMBuildLoad(builder, orig_gep, "load_orig");
	LLVMValueRef store_orig = LLVMBuildStore(builder, load_orig, alloca_gep);
	LLVMValueRef param_val = load_orig;
	if (LLVMGetTypeKind(LLVMTypeOf(load_orig)) == LLVMArrayTypeKind) {
		param_val = LLVMBuildExtractValue(builder, load_orig, 0, "extract_param_val");
	}

	// Create bound conditions
	LLVMValueRef upper_bound = LLVMConstReal(LLVMTypeOf(param_val), upper);
	LLVMValueRef lower_bound = LLVMConstReal(LLVMTypeOf(param_val), lower);

	LLVMValueRef upper_cond = LLVMBuildFCmp(builder, LLVMRealOLT, param_val,
	                                        upper_bound, "upper_bound");
	LLVMValueRef lower_cond = LLVMBuildFCmp(builder, LLVMRealOGE, param_val,
	                                        lower_bound, "lower_bound");

	// Get assumption intrinsic
	::std::string assume("llvm.assume");
	unsigned id = LLVMLookupIntrinsicID(assume.c_str(), assume.size());
	LLVMTypeRef assume_ty = LLVMInt1Type();
	LLVMValueRef assume_decl = LLVMGetIntrinsicDeclaration(f.getModule().get(), id, &assume_ty, 1);
	LLVMBuildCall(builder, assume_decl, &upper_cond, 1, "");
	LLVMBuildCall(builder, assume_decl, &lower_cond, 1, "");

	LLVMDisposeBuilder(builder);
}

int main(int argc, char **argv) {
	char c = -1;
	config conf;
	while ((c = getopt_long(argc, argv, "sp:l:u:h", options, NULL)) != -1) {
		switch (c) {
		case 'p': conf.param = ::std::string(optarg); break;
		case 'l': conf.lower_bound = ::std::stod(optarg); break;
		case 'u': conf.upper_bound = ::std::stod(optarg); break;
		default:
			::std::cerr << "Unknown option: " << argv[optind - 1]
			            << ::std::endl;
		case 'h':
			::std::cerr << "Available options:\n";
			::std::cerr << "\t-p,--param\t\tParameter name\n";
			::std::cerr << "\t-l,--lower-bound\t\tParameter lower bound\n";
			::std::cerr << "\t-u,--upper-bound\t\tParameter upper bound\n";
			return c == 'h' ? 0 : 1;
		}
	}
	if (conf.param.empty()) {
		::std::cerr << "ERROR: No parameter name provided!\n";
		return 2;
	}

	::std::deque<::std::pair<Module,::std::string>> modules;
	for (int i = optind; i < argc; ++i) {
		::std::cout << "Parsing file: " << argv[i] << "\n";
		modules.push_back(::std::make_pair(Module(argv[i]), argv[i]));
	}

	for (auto &m:modules) {
		auto res = find_src_and_indices(m.first, conf.param);
		int_seq &indices = res.second;
		if (!res.second.empty()) {
			apply_assumption(res.first, res.second, conf.lower_bound, conf.upper_bound);
			LLVMPrintModuleToFile(m.first.get(), m.second.c_str(), nullptr);
		}
	}

	return 0;
}
