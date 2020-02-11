/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Parser.hpp"

namespace parser
{

Errors parse( const srcfile_t & src, lex::toks_t & toks,
	      ptree_t * & ptree, phelper_t & ph, const size_t begin )
{
	std::vector< const stmt_base_t * > stmts;
	if( parse_block( ph, ( stmt_base_t * & )ptree, false ) != E_OK ) goto fail;
	return E_OK;
fail:
	delete ptree;
	return E_PARSE_FAIL;
}

}
