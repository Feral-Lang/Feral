/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef COMPILER_ARGS_HPP
#define COMPILER_ARGS_HPP

#include <vector>
#include <string>

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

size_t parse( const int argc, const char ** argv, std::vector< std::string > & args );

}

#endif // COMPILER_ARGS_HPP