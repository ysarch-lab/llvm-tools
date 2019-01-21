#include <getopt.h>

#include <deque>
#include <iostream>
#include <string>

#include "Module.h"

static const struct option options[] = {
	{"function", required_argument, NULL, 'f'},
	{"help", required_argument, NULL, 'h'},
};

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
		size_t count = 0;
		for (auto bbi = f.begin(); bbi != f.end(); ++bbi)
			for (auto ii = bbi->begin(); ii != bbi->end(); ++ii)
				++count;
		::std::cout << "FOUND: " << f << " (" << count << ")" << "\n";
	}

	return 0;
}
