#pragma once

#include <deque>
#include <string>
#include <unordered_map>

#include <llvm-c/Core.h>

#include "BasicBlock.h"
#include "Instruction.h"

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

static ::std::string get_pretty_name(const BasicBlock &bb)
{
	static ::std::unordered_map<::std::string, ::std::string> pretty_names_;

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
