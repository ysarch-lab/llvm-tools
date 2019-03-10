#include <getopt.h>

#include <cassert>
#include <deque>
#include <iostream>
#include <string>
#include <vector>

#include <cgraph.h>
#include <gvc.h>

#include "Module.h"

static const struct option options[] = {
	{"function", required_argument, NULL, 'f'},
	{"help", required_argument, NULL, 'h'},
};


static void find_loc_md_node(LLVMValueRef node, ::std::deque<::std::string> &strs)
{
	unsigned len = 0;
	const char *str = LLVMGetMDString(node, &len);

	if (str != nullptr) {
		assert(len > 0);
		::std::string md_string(str, len);
		// filter out file names
		if (md_string.substr(md_string.size() - 3, 3) != ".py" &&
		    md_string[0] != '/' && // Directory location
		    md_string[0] != '<' && // builtin location
		    md_string != "PsyNeuLink") // Generator name
			strs.push_front(::std::move(md_string));
		return;
	}

	const unsigned ops = LLVMGetMDNodeNumOperands(node);
	::std::vector<LLVMValueRef> operands(ops, nullptr);

	LLVMGetMDNodeOperands(node, operands.data());
	for (auto op:operands) {
		if (op) // Why would it be null?
			find_loc_md_node(op, strs);
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

static void graph_function(Function &f, Agraph_t *g)
{
	for (auto bbi = f.begin(); bbi != f.end(); ++bbi) {
		::std::deque<::std::string> names;
		for (auto ii = bbi->begin(); ii != bbi->end(); ++ii) {
			auto locs = get_inst_dloc(*ii);
			::std::string combined;
			for (auto &loc: locs) {
				if (combined.size() > 0)
					combined += ':';
				combined += loc;
			}
			names.push_back(combined);
		}
		::std::string name = bbi->getName();
		auto my_node = agnode(g, const_cast<char*>(name.c_str()), 1);
		for (auto succi = bbi->successor_begin();
		     succi != bbi->successor_end(); ++ succi) {
			::std::string name = succi->getName();
			auto succ_node = agnode(g, const_cast<char*>(name.c_str()), 1);
			agedge(g, my_node, succ_node, nullptr, 1);
		}
	}
}


int main(int argc, char **argv) {
	::std::string func_name;
	char c = -1;
	while ((c = getopt_long(argc, argv, "sf:h", options, NULL)) != -1) {
		switch (c) {
		case 'f': func_name = ::std::string(optarg); break;
		default:
			::std::cerr << "Unknown option: " << argv[optind - 1]
			            << ::std::endl;
		case 'h':
			::std::cerr << "Available options:\n";
			::std::cerr << "\t-f,--function\t\tfunction name (prefix) to analyze\n";
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

	GVC_t *ctx = gvContext();
	for (auto &f:func_stack) {
		Agraph_t *g = agopen(const_cast<char*>(f.getName().c_str()),
		                     Agstrictdirected, nullptr);
		graph_function(f, g);
		gvLayout(ctx, g, "dot");
		gvRenderFilename(ctx, g, "png", (f.getName() + ".png").c_str());
		gvFreeLayout(ctx, g);
		agclose(g);
	}
	gvFreeContext(ctx);

	return 0;
}
