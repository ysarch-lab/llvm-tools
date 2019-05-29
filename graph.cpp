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

static const struct option options[] = {
	{"function", required_argument, NULL, 'f'},
	{"pretty_names", no_argument, NULL, 'p'},
	{"control-flow", no_argument, NULL, 'c'},
	{"data-flow", no_argument, NULL, 'd'},
	{"memory-flow", no_argument, NULL, 'm'},
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

static ::std::unordered_map<::std::string, ::std::string> pretty_names_;

static ::std::string get_pretty_name(const BasicBlock &bb)
{
	::std::string name = bb.getName();
	::std::string &pretty_name = pretty_names_[name];

	if (!pretty_name.empty())
		return pretty_name;

	::std::deque<::std::string> prefix;
	for (auto ii = bb.begin(); ii != bb.end(); ++ii) {
		auto locs = get_inst_dloc(*ii);
		if (locs.size() == 1 && locs[0] == "unknown")
			continue;
		if (prefix.empty()) {
			prefix.swap(locs);
			continue;
		}
		for (int i = 0; i < ::std::min(prefix.size(), locs.size()); ++i) {
			if (locs[i] != prefix[i]) {
				prefix.erase(prefix.begin() + i, prefix.end());
				break;
			}
		}
	}
	if (prefix.size() == 0)
		pretty_name = name;
	else
		pretty_name = prefix.back();
	return pretty_name;
}

struct config{
	bool pretty_names = false;
	bool control_flow = false;
	bool data_flow = false;
	bool mem_flow = false;
};

static void graph_function(Function &f, Agraph_t *g, const config &c)
{
	for (auto bbi = f.begin(); bbi != f.end(); ++bbi) {

		::std::unordered_set<BasicBlock> preds;
		::std::unordered_set<BasicBlock> mem_preds;

		// Iterate over all instructions in the BB
		for (auto ii = bbi->begin(); ii != bbi->end(); ++ii) {
			// Iterate over all operands of an instruction
			for (auto opi = ii->op_begin(); opi != ii->op_end(); ++opi) {
				if (opi.isInstruction())
					preds.insert(BasicBlock::fromIntruction(*opi));
				else {
					auto val = opi.value();
					if (!LLVMIsConstant(val) &&
					    !LLVMIsAArgument(val) &&
					    !LLVMIsABasicBlock(val))
						::std::cerr << "UNKNOWN operand\n";
				}
			}
			if (ii->getOpcode() == LLVMLoad) {
				auto ptr = ii->getOperandInstruction(0);
				for (auto usei = ptr.use_begin(); usei != ptr.use_end(); ++usei)
					if (usei.isInstruction()) {
						if (usei->getOpcode() == LLVMStore)
							mem_preds.insert(BasicBlock::fromIntruction(*usei));
					} else
						::std::cerr << "USE IS NOT AN INSTRUCTION!!\n";
			}
		}

		::std::string name = c.pretty_names ? get_pretty_name(*bbi) : bbi->getName();
		auto my_node = agnode(g, const_cast<char*>(name.c_str()), 1);
		// Add data edges
		for (auto &pred:preds) {
			if (!c.data_flow)
				break;
			if (pred == *bbi)
				continue;
			::std::string name = c.pretty_names ? get_pretty_name(pred) : pred.getName();
			auto pred_node = agnode(g, const_cast<char*>(name.c_str()), 1);
			auto edge = agedge(g, pred_node, my_node, nullptr, 1);
			agsafeset(edge, "color", "red", "black");
		}

		// Add mem edges
		for (auto &mem_pred:mem_preds) {
			if (!c.mem_flow)
				break;
			if (mem_pred == *bbi)
				continue;
			::std::string name = c.pretty_names ? get_pretty_name(mem_pred) : mem_pred.getName();
			auto pred_node = agnode(g, const_cast<char*>(name.c_str()), 1);
			auto edge = agedge(g, pred_node, my_node, nullptr, 1);
			agsafeset(edge, "color", "green", "black");
		}

		// Add CF edges
		for (auto succi = bbi->successor_begin(),
		          succe = bbi->successor_end();
		     succi != succe && c.control_flow; ++ succi) {
			::std::string name = c.pretty_names ? get_pretty_name(*succi) : succi->getName();
			auto succ_node = agnode(g, const_cast<char*>(name.c_str()), 1);
			agedge(g, my_node, succ_node, nullptr, 1);
		}
	}
}


int main(int argc, char **argv) {
	::std::string func_name;
	char c = -1;
	config conf;
	while ((c = getopt_long(argc, argv, "sf:pcdmh", options, NULL)) != -1) {
		switch (c) {
		case 'f': func_name = ::std::string(optarg); break;
		case 'p': conf.pretty_names = true; break;
		case 'c': conf.control_flow = true; break;
		case 'd': conf.data_flow = true; break;
		case 'm': conf.mem_flow = true; break;
		default:
			::std::cerr << "Unknown option: " << argv[optind - 1]
			            << ::std::endl;
		case 'h':
			::std::cerr << "Available options:\n";
			::std::cerr << "\t-f,--function\t\tfunction name (prefix) to analyze\n";
			::std::cerr << "\t-p,--pretty-names\t\tUse debug location to determined basic block name\n";
			::std::cerr << "\t-c,--control-flow\t\tPrint control flow edges\n";
			::std::cerr << "\t-d,--data-flow\t\tPrint data flow edges\n";
			::std::cerr << "\t-m,--mem-flow\t\tPrint memory flow edges\n";
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
		gvRenderFilename(ctx, g, "png", (f.getName() + ".png").c_str());
		gvFreeLayout(ctx, g);
		agclose(g);
	}
	gvFreeContext(ctx);

	return 0;
}
