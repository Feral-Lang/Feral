/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Internal.hpp"

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
		ph.fail( "statement of type '%s' expects semicolon after the keyword, found: '%s'",
			 TokStrs[ sost->type ], TokStrs[ ph.peakt() ] );
		goto fail;
	}

	if( sost->type == TOK_RETURN && ph.accept( TOK_LBRACE ) ) {
		ph.fail( "'return' statement expects a semicolon or expression after the keyword, not a block" );
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
		ph.fail( "expected semicolon to denote end of statement '%s', but found: '%s'",
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
