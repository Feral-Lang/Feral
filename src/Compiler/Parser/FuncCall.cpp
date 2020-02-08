/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Internal.hpp"

Errors parse_fn_call_args( phelper_t & ph, stmt_base_t * & loc )
{
	// assn_args = kw args
	std::vector< const stmt_base_t * > args, assn_args;
	stmt_base_t * expr = nullptr;

	size_t idx = ph.idx();
begin:
	if( ph.acceptd() && ph.peakt( 1 ) == TOK_ASSN ) {
		const lex::tok_t * lhs = ph.peak();
		stmt_base_t * rhs = nullptr;
		ph.next(); ph.next();
		if( parse_expr_13( ph, rhs ) != E_OK ) {
			goto fail;
		}
		assn_args.push_back( new stmt_fn_assn_arg_t( lhs, rhs ) );
	} else if( parse_expr_13( ph, expr ) == E_OK ) {
		args.push_back( expr );
	} else {
		ph.fail( "failed to parse function call args" );
		goto fail;
	}

	if( ph.accept( TOK_COMMA ) ) {
		ph.next();
		goto begin;
	}

	loc = new stmt_fn_call_args_t( args, assn_args, idx );
	return E_OK;
fail:
	for( auto & arg : args ) delete arg;
	return E_PARSE_FAIL;
}
