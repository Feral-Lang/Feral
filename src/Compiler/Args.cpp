/*
	MIT License

	Copyright (c) 2020 Feral Language repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "Compiler/Args.hpp"

#include <cstring>

const size_t OPT_A = 1 << 0;
const size_t OPT_B = 1 << 1; // show byte code
const size_t OPT_C = 1 << 2; // (byte) compile
const size_t OPT_D = 1 << 3; // dry run (no execute)
const size_t OPT_E = 1 << 4; // REPL (eval)
const size_t OPT_F = 1 << 5;
const size_t OPT_G = 1 << 6;
const size_t OPT_H = 1 << 7;
const size_t OPT_I = 1 << 8;
const size_t OPT_L = 1 << 9;
const size_t OPT_P = 1 << 10; // show parse tree
const size_t OPT_R = 1 << 11; // recursively show everything (ex. FrontEnd->VM->Import->FrontEnd...)
const size_t OPT_S = 1 << 12;
const size_t OPT_T = 1 << 13; // show tokens
const size_t OPT_V = 1 << 14; // show version
const size_t OPT_1 = 1 << 15;

namespace args
{
size_t parse(const int argc, const char **argv, std::unordered_map<std::string, std::string> &args,
	     std::vector<std::string> &code_args)
{
	bool main_done = false;
	size_t flags   = 0;
	char prev_flag = '\0';

	for(int i = 1; i < argc; ++i) {
		if(main_done) {
			code_args.push_back(argv[i]);
			continue;
		}

		size_t len = strlen(argv[i]);

		if(len > 2 && argv[i][0] == '-' && argv[i][1] == '-') {
			args.emplace("__long_opt__", argv[i]);
			continue;
		}

		if(argv[i][0] != '-') {
			args.emplace("__main__", argv[i]);
			main_done = true;
			continue;
		}

		for(size_t j = 1; j < len; ++j) {
			switch(argv[i][j]) {
			case 'a': flags |= OPT_A; break;
			case 'b': flags |= OPT_B; break;
			case 'c': flags |= OPT_C; break;
			case 'd': flags |= OPT_D; break;
			case 'e': flags |= OPT_E; break;
			case 'f': flags |= OPT_F; break;
			case 'g': flags |= OPT_G; break;
			case 'h': flags |= OPT_H; break;
			case 'i': flags |= OPT_I; break;
			case 'l': flags |= OPT_L; break;
			case 'p': flags |= OPT_P; break;
			case 'r': flags |= OPT_R; break;
			case 's': flags |= OPT_S; break;
			case 't': flags |= OPT_T; break;
			case 'v': flags |= OPT_V; break;
			case '1': flags |= OPT_1; break;
			}
			prev_flag = argv[i][j];
		}
	}

	// dry run if compiled flag exists (no actual execution of program)
	if(flags & OPT_C) flags |= OPT_D;

	return flags;
}

} // namespace args
