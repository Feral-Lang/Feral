/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Internal.hpp"

Errors parse_fn_decl( phelper_t & ph, stmt_base_t * & loc )
{
	size_t idx = ph.peak()->pos;
	bool args_done = false;
	stmt_base_t * args = nullptr, * block = nullptr;

	if( !ph.accept( TOK_FN ) ) {
		ph.fail( "function declaration parsing requires function keyword" );
		goto fail;
	}
	ph.next();

	if( !ph.accept( TOK_LPAREN ) ) {
		ph.fail( "expected left parenthesis here for function declaration" );
		goto fail;
	}
	ph.next();

rparen:
	if( ph.accept( TOK_RPAREN ) ) {
		ph.next();
		goto post_args;
	}
	if( args_done ) {
		ph.fail( "expected right parenthesis for enclosing function arguments" );
		goto fail;
	}
	if( parse_fn_decl_args( ph, args ) != E_OK ) {
		goto fail;
	}
	args_done = true;
	goto rparen;

post_args:
	if( parse_block( ph, block ) != E_OK ) {
		fprintf( stderr, "failed to parse block for function\n" );
		goto fail;
	}

	loc = new stmt_fn_def_t( ( stmt_fn_def_args_t * )args, ( stmt_block_t * )block, idx );
	return E_OK;
fail:
	if( args ) delete args;
	if( block ) delete block;
	return E_PARSE_FAIL;
}

Errors parse_fn_decl_args( phelper_t & ph, stmt_base_t * & loc )
{
	std::vector< const stmt_base_t * > args;
	const lex::tok_t * kw_arg = nullptr, * va_arg = nullptr;
	stmt_base_t * expr = nullptr;

	size_t idx = ph.peak()->pos;
begin:
	if( va_arg ) {
		ph.fail( "cannot have any argument after variadic arg declaration" );
		goto fail;
	}
	if( ph.accept( TOK_ATOM ) ) { // check kw arg
		if( kw_arg ) {
			ph.fail( "function can't have multiple keyword args (previous: %s)",
				 kw_arg->data.c_str() );
			goto fail;
		}
		kw_arg = ph.peak();
		ph.next();
	} else if( ph.acceptd() && ( ph.peakt( 1 ) == TOK_ASSN || ph.peakt( 1 ) == TOK_TDOT ) ) { // since ATOM is checked above, won't be bothered with it anymore
		if( ph.peakt( 1 ) == TOK_ASSN ) { // it's a default assignment
			const lex::tok_t * lhs = ph.peak();
			stmt_base_t * rhs = nullptr;
			ph.next(); ph.next();
			if( parse_expr_13( ph, rhs ) != E_OK ) {
				goto fail;
			}
			args.push_back( new stmt_fn_assn_arg_t( lhs, rhs ) );
		} else if( ph.peakt( 1 ) == TOK_TDOT ) { // perhaps a variadic
			va_arg = ph.peak();
			ph.next(); ph.next();
		}
	} else if( parse_expr_13( ph, expr ) == E_OK ) {
		args.push_back( expr );
	} else {
		ph.fail( "failed to parse function def args" );
		goto fail;
	}

	if( ph.accept( TOK_COMMA ) ) {
		ph.next();
		goto begin;
	}

	loc = new stmt_fn_def_args_t( args, kw_arg, va_arg, idx );
	return E_OK;
fail:
	for( auto & arg : args ) delete arg;
	return E_PARSE_FAIL;
}
