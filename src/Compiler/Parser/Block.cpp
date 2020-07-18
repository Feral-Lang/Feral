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

Errors parse_block( phelper_t & ph, stmt_base_t * & loc, const bool with_brace )
{
	std::vector< const stmt_base_t * > stmts;

	size_t idx = ph.peak()->pos;

	if( with_brace ) {
		if( !ph.accept( TOK_LBRACE ) ) {
			err::set( E_PARSE_FAIL, ph.peak()->pos, "Expected block to begin with left brace, found '%s'",
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
		} else {
			if( parse_expr_cols_or_rbrace( ph, stmt ) == E_OK ) {
				if( stmt->type() != GT_EXPR ) {
					stmt = new stmt_expr_t( stmt, nullptr, nullptr, stmt->idx() );
				}
				static_cast< stmt_expr_t * >( stmt )->set_with_cols( ph.peakt() != TOK_RBRACE );
				if( ph.peakt() == TOK_COLS ) ph.next();
			} else {
				goto fail;
			}
		}
		stmts.push_back( stmt );
	}

	if( with_brace ) {
		if( !ph.accept( TOK_RBRACE ) ) {
			err::set( E_PARSE_FAIL, idx, "Expected this block to end with a right brace, found '%s'",
				 TokStrs[ ph.peakt() ] );
			goto fail;
		}
		ph.next();
	} else if( ph.peakt() == TOK_RBRACE ) {
		err::set( E_PARSE_FAIL, idx, "cannot have end braces for top level block" );
		goto fail;
	}

	loc = new stmt_block_t( stmts, idx );
	static_cast< stmt_block_t * >( loc )->set_no_brace( !with_brace );
	return E_OK;
fail:
	for( auto & stmt : stmts ) delete stmt;
	return E_PARSE_FAIL;
}
