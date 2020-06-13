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

#include "Compiler/CodeGen/Internal.hpp"

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
		err::set( E_PARSE_FAIL, ph.peak()->pos, "expected left brace to begin body of loop, found: '%s'",
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
