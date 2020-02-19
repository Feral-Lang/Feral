/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Internal.hpp"

Errors parse_for( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * init = nullptr, * cond = nullptr, * incr = nullptr;
	stmt_base_t * body = nullptr;

	size_t idx = ph.peak()->pos;
	ph.next();
init:
	if( ph.accept( TOK_COLS ) ) {
		ph.next();
		goto cond;
	}

	// both var_decl and expr_cols include semicolon
	if( ph.accept( TOK_LET ) ) {
		if( parse_var_decl( ph, init ) != E_OK ) goto fail;
	} else if( parse_expr_cols( ph, init ) != E_OK ) {
		goto fail;
	}
cond:
	if( ph.accept( TOK_COLS ) ) {
		ph.next();
		goto incr;
	}

	if( parse_expr_cols( ph, cond ) != E_OK ) {
		goto fail;
	}
incr:
	if( ph.accept( TOK_LBRACE ) ) {
		goto body;
	}

	if( parse_expr( ph, incr ) != E_OK ) {
		goto fail;
	}
body:
	if( !ph.accept( TOK_LBRACE ) ) {
		ph.fail( "expected left brace to begin body of loop, found: '%s'",
			 TokStrs[ ph.peakt() ] );
		goto fail;
	}
	if( parse_block( ph, body ) != E_OK ) {
		goto fail;
	}

	loc = new stmt_for_t( init, cond, incr, body, idx );
	return E_OK;
fail:
	if( init ) delete init;
	if( cond ) delete cond;
	if( incr ) delete incr;
	if( body ) delete body;
	return E_PARSE_FAIL;
}
