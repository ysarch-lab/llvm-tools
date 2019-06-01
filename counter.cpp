#include <getopt.h>

#include <cassert>
#include <deque>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Module.h"
#include "debug_location.h"

static const struct option options[] = {
	{"function", required_argument, NULL, 'f'},
	{"skip-non-compute", required_argument, NULL, 's'},
	{"help", required_argument, NULL, 'h'},
};

using counts_t = ::std::unordered_map<::std::string, size_t>;

static void analyze_function(Function &f, counts_t &counts, bool skip_non_compute)
{
	for (auto bbi = f.begin(); bbi != f.end(); ++bbi)
		for (auto ii = bbi->begin(); ii != bbi->end(); ++ii) {
			if (skip_non_compute &&
			    (ii->getOpcode() == LLVMLoad ||
			     ii->getOpcode() == LLVMStore ||
			     ii->getOpcode() == LLVMBitCast ||
			     ii->getOpcode() == LLVMGetElementPtr ||
			     ii->getOpcode() == LLVMPHI))
				continue;

			auto locs = get_inst_dloc(*ii);
			::std::string combined;
			for (auto &loc: locs) {
				if (combined.size() > 0)
					combined += ':';
				combined += loc;
			}
			counts[combined] += 1;
			if (ii->getOpcode() == LLVMCall ||
			    ii->getOpcode() == LLVMInvoke) {
				Function f(ii->getCalledValue());
				analyze_function(f, counts, skip_non_compute);
			}
		}
}

int main(int argc, char **argv) {
	::std::string func_name;
	char c = -1;
	bool skip_non_compute = false;
	while ((c = getopt_long(argc, argv, "sf:h", options, NULL)) != -1) {
		switch (c) {
		case 's': skip_non_compute = true; break;
		case 'f': func_name = ::std::string(optarg); break;
		default:
			::std::cerr << "Unknown option: " << argv[optind - 1]
			            << ::std::endl;
		case 'h':
			::std::cerr << "Available options:\n";
			::std::cerr << "\t-f,--function\t\tfunction name (prefix) to analyze\n";
			::std::cerr << "\t-s,--skip-non-compute\tignore non-compute operations (LOAD, STORE, GEP, BITCAST, PHI)\n";
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
		counts_t counts;
		analyze_function(f, counts, skip_non_compute);
		size_t total = 0;
		for (auto &count: counts) {
			::std::cout << f << ": " << count.first << ": " << count.second << "\n";
			total += count.second;
		}
		::std::cout << f << ": total: " << total << "\n";
	}

	return 0;
}
