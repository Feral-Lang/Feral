/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef COMPILER_PARSER_HPP
#define COMPILER_PARSER_HPP

#include "Parser/Internal.hpp"

namespace parser
{

Errors parse( phelper_t & ph, lex::toks_t & toks, ptree_t * & ptree, const size_t begin = 0 );

}

#endif // COMPILER_PARSER_HPP
