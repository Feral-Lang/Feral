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

#ifndef COMPILER_ARGS_HPP
#define COMPILER_ARGS_HPP

#include <vector>
#include <string>
#include <unordered_map>

// Option bit masks
extern const size_t OPT_A;
extern const size_t OPT_B; // show byte code
extern const size_t OPT_C; // (byte) compile
extern const size_t OPT_D; // dry run (no execute)
extern const size_t OPT_E; // REPL (eval)
extern const size_t OPT_F;
extern const size_t OPT_G;
extern const size_t OPT_H;
extern const size_t OPT_I;
extern const size_t OPT_L;
extern const size_t OPT_P; // show parse tree
extern const size_t OPT_R; // recursively show everything (ex. FrontEnd->VM->Import->FrontEnd...)
extern const size_t OPT_S;
extern const size_t OPT_T; // show tokens
extern const size_t OPT_V; // show version
extern const size_t OPT_1;

namespace args
{

size_t parse( const int argc, const char ** argv,
	      std::unordered_map< std::string, std::string > & args,
	      std::vector< std::string > & code_args );

}

#endif // COMPILER_ARGS_HPP
