#include <getopt.h>

#include <deque>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Module.h"

static const struct option options[] = {
	{"function", required_argument, NULL, 'f'},
	{"help", required_argument, NULL, 'h'},
};

using counts_t = ::std::unordered_map<::std::string, size_t>;

static void find_loc_md_node(LLVMValueRef node, ::std::deque<::std::string> &strs, int depth = 0)
{
	LLVMMetadataRef MD = LLVMValueAsMetadata(node);
	unsigned len = 12;
	const char * str = LLVMGetMDString(node, &len);
	const unsigned ops = LLVMGetMDNodeNumOperands(node);
#if 0
	for (int i = 0; i < depth; ++i)
		::std::cerr << "\t";
	::std::cerr << node << ": str: " << ::std::string(str, len)  << " (" << len << "):" << ops << " " << MD << " | " << (void*)str <<"\n";
#endif

	if (str != nullptr) {
		// Primary location is at depth 2. Other locations are the
		// results of 'inlined at' reference
		if (depth == 2)
			strs.push_back({str, len});
		return;
	}

	::std::vector<LLVMValueRef> operands(ops, nullptr);
	LLVMGetMDNodeOperands(node, operands.data());
	for (auto op:operands) {
		if (op) // Why whould it be null?
			find_loc_md_node(op, strs, depth + 1);
	}
}

static ::std::deque<::std::string> get_inst_dloc(const Instruction &i)
{
	static const unsigned dbg_kind_id = LLVMGetMDKindID("dbg", 3);
	LLVMValueRef MDV = i.getMetadata(dbg_kind_id);

	::std::deque<::std::string> strs;
	if (MDV == NULL) {
		strs.push_back("unknown");
	} else {
		find_loc_md_node(MDV, strs);
	}
	return strs;
}

static counts_t analyze_function(Function &f)
{
	counts_t counts;
	for (auto bbi = f.begin(); bbi != f.end(); ++bbi)
		for (auto ii = bbi->begin(); ii != bbi->end(); ++ii) {
			auto locs = get_inst_dloc(*ii);
			for (auto &loc: locs)
				counts[loc] += 1;
			// TODO: Consider calls to other functions
		}
	return counts;
}


int main(int argc, char **argv) {
	::std::string func_name;
	char c = -1;
	while ((c = getopt_long(argc, argv, "f:h", options, NULL)) != -1) {
		switch (c) {
		case 'f': func_name = ::std::string(optarg); break;
		default:
			::std::cerr << "Unknown option: " << argv[optind - 1]
			            << ::std::endl;
		case 'h':
			return c == 'h' ? 0 : 1;
		}
	}
	if (func_name.empty()) {
		::std::cerr << "ERROR: No function name provided!\n";
		return 2;
	}

	::std::deque<Module> modules;
	for (int i = optind; i < argc; ++i) {
		::std::cout << "Parsing file: " << argv[i] << "\n";
		modules.push_back(Module(argv[i]));
	}

	::std::deque<Function> func_stack;
	for (auto &m:modules)
		for (auto it = m.func_begin(); it != m.func_end(); ++it) {
			auto mm = ::std::mismatch(func_name.begin(),
			                          func_name.end(),
						  it->getName().begin());
			if (mm.first == func_name.end())
				func_stack.push_back(*it);
		}

	for (auto &f:func_stack) {
		counts_t counts = analyze_function(f);
		size_t total = 0;
		for (auto &count: counts) {
			::std::cout << f << ": " << count.first << ": " << count.second << "\n";
			total += count.second;
		}
		::std::cout << f << ": total: " << total << "\n";
	}

	return 0;
}
