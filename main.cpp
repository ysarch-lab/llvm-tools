#include <getopt.h>

#include <deque>
#include <iostream>

#include "Module.h"

static const struct option options[] = {
	{"function", required_argument, NULL, 'f'},
	{"help", required_argument, NULL, 'h'},
};

int main(int argc, char **argv) {
	const char *func_name = NULL;
	char c = -1;
	while ((c = getopt_long(argc, argv, "f:h", options, NULL)) != -1) {
		switch (c) {
		case 'f': func_name = optarg; break;
		default:
			::std::cerr << "Unknown option: " << argv[optind - 1]
			            << ::std::endl;
		case 'h':
			return c == 'h' ? 0 : 1;
		}
	}
	if (func_name == NULL) {
		::std::cerr << "ERROR: No function name provided!\n";
		return 2;
	}

	::std::deque<Module> modules;
	for (int i = optind; i < argc; ++i) {
		::std::cout << "Parsing file: " << argv[i] << "\n";
		modules.push_back(Module(argv[i]));
	}

	for (auto &m:modules)
		for (auto it = m.func_begin(); it != m.func_end(); ++it)
			::std::cout << *it << "\n";

	::std::cout << "Hello " << func_name << "\n";
	return 0;
}
