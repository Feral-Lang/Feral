/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Compiler/Parser.hpp"

namespace parser
{

Errors parse( phelper_t & ph, lex::toks_t & toks, ptree_t * & ptree, const size_t begin )
{
	if( parse_block( ph, ( stmt_base_t * & )ptree, false ) != E_OK ) goto fail;
	return E_OK;
fail:
	delete ptree;
	return E_PARSE_FAIL;
}

}
