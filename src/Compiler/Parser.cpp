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
	      ptree_t & ptree, phelper_t & ph, const size_t begin )
{
	std::vector< const stmt_base_t * > stmts;
	while( ph.valid() ) {
		stmt_base_t * stmt = nullptr;
		if( parse_block( ph, stmt, false ) != E_OK ) goto fail;
		ptree.push_back( stmt );
	}
	return E_OK;
fail:
	for( auto & p : ptree ) {
		delete p;
	}
	ptree.clear();
	return E_PARSE_FAIL;
}

}
