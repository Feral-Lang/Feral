/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Internal.hpp"

Errors parse_block( phelper_t & ph, stmt_base_t * & loc, const bool with_brace )
{
	std::vector< const stmt_base_t * > stmts;

	size_t idx = ph.peak()->pos;

	if( with_brace ) {
		if( !ph.accept( TOK_LBRACE ) ) {
			ph.fail( "Expected block to begin with left brace, found '%s'",
				 TokStrs[ ph.peakt() ] );
			goto fail;
		}
		ph.next();
	}

	while( ph.valid() && ( !with_brace || ph.peakt() != TOK_RBRACE ) ) {
		stmt_base_t * stmt = nullptr;
		if( ph.accept( TOK_LET ) ) {
			if( parse_var_decl( ph, stmt ) != E_OK ) goto fail;
		} else if( ph.accept( TOK_CONTINUE, TOK_BREAK, TOK_RETURN ) ) {
			if( parse_single_operand_stmt( ph, stmt ) != E_OK ) goto fail;
		} else if( ph.accept( TOK_IF ) ) {
			if( parse_conditional( ph, stmt ) != E_OK ) goto fail;
		} else if( ph.accept( TOK_FOR ) ) {
			if( ph.peakt( 1 ) == TOK_IDEN && ph.peakt( 2 ) == TOK_IN ) {
				if( parse_foreach( ph, stmt ) != E_OK ) goto fail;
			} else {
				if( parse_for( ph, stmt ) != E_OK ) goto fail;
			}
		} else if( ph.accept( TOK_WHILE ) ) {
			if( parse_while( ph, stmt ) != E_OK ) goto fail;
		} else if( ph.accept( TOK_LBRACE ) ) {
			if( parse_block( ph, stmt ) != E_OK ) goto fail;
		} else if( parse_expr_cols( ph, stmt ) != E_OK ) {
			goto fail;
		}
		stmts.push_back( stmt );
	}

	if( with_brace ) {
		if( !ph.accept( TOK_RBRACE ) ) {
			ph.fail( idx, "Expected this block to end with a right brace, found '%s'",
				 TokStrs[ ph.peakt() ] );
			goto fail;
		}
		ph.next();
	}

	loc = new stmt_block_t( stmts, idx );
	return E_OK;
fail:
	for( auto & stmt : stmts ) delete stmt;
	return E_PARSE_FAIL;
}
