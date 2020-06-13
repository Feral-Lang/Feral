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

Errors parse_single_operand_stmt( phelper_t & ph, stmt_base_t * & loc )
{
	const lex::tok_t * sost = ph.peak();
	stmt_base_t * operand = nullptr;

	ph.next();

	if( ph.accept( TOK_COLS ) ) {
		ph.next();
		goto done;
	}

	if( sost->type == TOK_CONTINUE || sost->type == TOK_BREAK ) {
		err::set( E_PARSE_FAIL, ph.peak()->pos, "statement of type '%s' expects semicolon after the keyword, found: '%s'",
			 TokStrs[ sost->type ], TokStrs[ ph.peakt() ] );
		goto fail;
	}

	if( sost->type == TOK_RETURN && ph.accept( TOK_LBRACE ) ) {
		err::set( E_PARSE_FAIL, ph.peak()->pos, "'return' statement expects a semicolon or expression after the keyword, not a block" );
		goto fail;
	}

	if( ph.accept( TOK_LBRACE ) ) {
		if( parse_block( ph, operand ) != E_OK ) goto fail;
		goto done;
	}

	if( parse_expr_15( ph, operand ) != E_OK ) {
		goto fail;
	}

	if( !ph.accept( TOK_COLS ) ) {
		err::set( E_PARSE_FAIL, ph.peak()->pos, "expected semicolon to denote end of statement '%s', but found: '%s'",
			 TokStrs[ sost->type ], TokStrs[ ph.peakt() ] );
		goto fail;
	}
	ph.next();

done:
	loc = new stmt_single_operand_stmt_t( sost, operand );
	return E_OK;
fail:
	if( operand ) delete operand;
	return E_PARSE_FAIL;
}
