#include <getopt.h>

#include <cassert>
#include <deque>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <cgraph.h>
#include <gvc.h>

#include "Module.h"
#include "debug_location.h"

static const struct option options[] = {
	{"function", required_argument, NULL, 'f'},
	{"pretty_names", no_argument, NULL, 'p'},
	{"skip_stores", no_argument, NULL, 's'},
	{"skip_loads", no_argument, NULL, 'l'},
	{"skip_geps", no_argument, NULL, 'g'},
	{"name_limit", required_argument, NULL, 'n'},
	{"auxiliary_name_limit", required_argument, NULL, 'a'},
	{"name_count", required_argument, NULL, 'c'},
	{"help", no_argument, NULL, 'h'},
};

struct config{
	bool pretty_names = false;
	bool skip_stores = false;
	bool skip_loads = false;
	bool skip_geps = false;
	int primary_name_limit = -1;
	int auxiliary_name_limit = -1;
	int name_count = 1;
};

static ::std::string get_inst_name(const Instruction &i, const config &c)
{
	auto locs = get_inst_dloc(i);
	if (locs.empty() or !c.pretty_names)
		return i.getName();
	if (locs.size() == 1)
		locs.front() += i.getName();

	if (c.primary_name_limit > -1 && locs.size() > c.primary_name_limit)
		locs.resize(c.primary_name_limit);
	else if (c.auxiliary_name_limit > -1 && locs.size() > c.auxiliary_name_limit)
		locs.resize(c.auxiliary_name_limit);
	while (locs.size() > c.name_count)
		locs.pop_front();
	::std::string name;
	for (auto &loc: locs) {
		if (name.size() > 0)
			name += ":";
		name += loc;
	}
	return name;
}

::std::deque<::std::string> get_dependecy_names(Instruction &i, const config &c)
{
	::std::deque<::std::string>  names;
	// Iterate over all operands of an instruction
	for (auto opi = i.op_begin(); opi != i.op_end(); ++opi) {
		if (opi.isInstruction()) {
			if (c.skip_loads && opi->getOpcode() == LLVMLoad)
				continue;

			// Expand dependencies of getelementptr op
			if (opi->getOpcode() == LLVMGetElementPtr) {
				auto ptr_names = get_dependecy_names(*opi, c);
				names.insert(names.end(), ptr_names.begin(), ptr_names.end());
			} else {
				::std::string op_name = get_inst_name(*opi, c);
				names.push_back(op_name);
			}
		}
	}
	return names;
}

static void graph_function(Function &f, Agraph_t *g, const config &c)
{
	// Iterate over all BBs in the function
	for (auto bbi = f.begin(); bbi != f.end(); ++bbi) {

		// Iterate over all instructions in the BB
		for (auto ii = bbi->begin(); ii != bbi->end(); ++ii) {
			if (c.skip_stores && ii->getOpcode() == LLVMStore)
				continue;
			if (c.skip_geps && ii->getOpcode() == LLVMGetElementPtr)
				continue;

			auto pred_names = get_dependecy_names(*ii, c);
			if (pred_names.empty())
				continue;

			::std::string name = get_inst_name(*ii, c);
			auto my_node = agnode(g, const_cast<char*>(name.c_str()), 1);
			if (name.find("Output_CIM") != ::std::string::npos)
				// attribute, value, default_value
				agsafeset(my_node, "color", "red", "black");
			for (auto &op_name:pred_names) {
				// Avoid self edges when using pretty names
				if (name == op_name && c.pretty_names)
					continue;
				auto pred_node = agnode(g, const_cast<char*>(op_name.c_str()), 1);
				if (op_name.find("Input_CIM") != ::std::string::npos)
					// attribute, value, default_value
					agsafeset(pred_node, "color", "green", "black");
				auto edge = agedge(g, pred_node, my_node, nullptr, 1);
				// attribute, value, default_value
				agsafeset(edge, "color", "blue", "black");
			}
		}

	}
}


int main(int argc, char **argv) {
	::std::string func_name;
	char c = -1;
	config conf;
	while ((c = getopt_long(argc, argv, "f:pslgn:a:c:h", options, NULL)) != -1) {
		switch (c) {
		case 'f': func_name = ::std::string(optarg); break;
		case 'p': conf.pretty_names = true; break;
		case 's': conf.skip_stores = true; break;
		case 'l': conf.skip_loads = true; break;
		case 'g': conf.skip_geps = true; break;
		case 'n': conf.primary_name_limit = ::std::stoi(optarg); break;
		case 'a': conf.auxiliary_name_limit = ::std::stoi(optarg); break;
		case 'c': conf.name_count = ::std::stoi(optarg); break;
		default:
			::std::cerr << "Unknown option: " << argv[optind - 1]
			            << ::std::endl;
		case 'h':
			::std::cerr << "Available options:\n";
			::std::cerr << "\t-f,--function\t\tfunction name (prefix) to analyze\n";
			::std::cerr << "\t-p,--pretty-names\t\tUse debug location to determined basic block name\n";
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
		graph_function(f, g, conf);
		gvLayout(ctx, g, "dot");
		gvRenderFilename(ctx, g, "pdf", (f.getModule().getName() + "_" + f.getName() + ".pdf").c_str());
		gvFreeLayout(ctx, g);
		agclose(g);
	}
	gvFreeContext(ctx);

	return 0;
}
