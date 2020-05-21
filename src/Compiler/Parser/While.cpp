/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Compiler/CodeGen/Internal.hpp"

Errors parse_while( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * expr = nullptr;
	stmt_base_t * body = nullptr;

	size_t idx = ph.peak()->pos;
	ph.next();

	if( parse_expr_15( ph, expr ) != E_OK ) {
		goto fail;
	}
	if( parse_block( ph, body ) != E_OK ) {
		goto fail;
	}

	loc = new stmt_while_t( expr, body, idx );
	return E_OK;
fail:
	if( expr ) delete expr;
	if( body ) delete body;
	return E_PARSE_FAIL;
}
