/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <unordered_map>

#include "Compiler/CodeGen/Internal.hpp"

Errors parse_fn_decl( phelper_t & ph, stmt_base_t * & loc )
{
	bool args_done = false;
	stmt_base_t * args = nullptr, * body = nullptr;
	size_t idx = ph.peak()->pos;

	if( !ph.accept( TOK_FN ) ) {
		err::set( E_PARSE_FAIL, ph.peak()->pos, "function declaration parsing requires function keyword" );
		goto fail;
	}
	ph.next();

	if( !ph.accept( TOK_LPAREN ) ) {
		err::set( E_PARSE_FAIL, ph.peak()->pos, "expected left parenthesis here for function declaration" );
		goto fail;
	}
	ph.next();

rparen:
	if( ph.accept( TOK_RPAREN ) ) {
		ph.next();
		goto post_args;
	}
	if( args_done ) {
		err::set( E_PARSE_FAIL, ph.peak()->pos, "expected right parenthesis for enclosing function arguments" );
		goto fail;
	}
	if( parse_fn_decl_args( ph, args ) != E_OK ) {
		goto fail;
	}
	args_done = true;
	goto rparen;

post_args:
	if( !ph.accept( TOK_LBRACE ) ) {
		err::set( E_PARSE_FAIL, ph.peak()->pos, "expected left brace for body of function, found: '%s'", TokStrs[ ph.peakt() ] );
	}
	if( parse_block( ph, body ) != E_OK ) {
		fprintf( stderr, "failed to parse block for function\n" );
		goto fail;
	}

	loc = new stmt_fn_def_t( ( stmt_fn_def_args_t * )args, ( stmt_block_t * )body, idx );
	return E_OK;
fail:
	if( args ) delete args;
	if( body ) delete body;
	return E_PARSE_FAIL;
}

Errors parse_fn_decl_args( phelper_t & ph, stmt_base_t * & loc )
{
	std::vector< const stmt_base_t * > args;
	const stmt_simple_t * kw_arg = nullptr, * va_arg = nullptr;
	stmt_base_t * expr = nullptr;
	std::unordered_map< std::string, size_t > done_assn_args;

	size_t idx = ph.peak()->pos;
begin:
	if( va_arg ) {
		err::set( E_PARSE_FAIL, ph.peak()->pos, "cannot have any argument after variadic arg declaration" );
		goto fail;
	}
	if( ph.accept( TOK_STR ) ) { // check kw arg
		if( kw_arg ) {
			err::set( E_PARSE_FAIL, ph.peak()->pos, "function can't have multiple keyword args (previous: %s)",
				 kw_arg->val()->data.c_str() );
			goto fail;
		}
		kw_arg = new stmt_simple_t( ph.peak() );
		ph.next();
	} else if( ph.accept( TOK_IDEN ) && ph.peakt( 1 ) != TOK_ASSN && ph.peakt( 1 ) != TOK_TDOT ) {
		if( done_assn_args.size() > 0 ) {
			err::set( E_PARSE_FAIL, ph.peak()->pos, "cannot have a simple parameter after default argument" );
			goto fail;
		}
		ph.sett( TOK_STR );
		args.push_back( new stmt_simple_t( ph.peak() ) );
		ph.next();
	} else if( ph.acceptd() && ( ph.peakt( 1 ) == TOK_ASSN || ph.peakt( 1 ) == TOK_TDOT ) ) { // since STR is checked above, won't be bothered with it anymore
		// but we still have to make IDEN data type to STR
		if( ph.accept( TOK_IDEN ) ) ph.sett( TOK_STR );
		if( ph.peakt( 1 ) == TOK_ASSN ) {
			const lex::tok_t * lhs = ph.peak();
			stmt_base_t * rhs = nullptr;
			ph.next(); ph.next();
			if( parse_expr_15( ph, rhs ) != E_OK ) {
				goto fail;
			}
			if( done_assn_args.find( lhs->data ) != done_assn_args.end() ) {
				err::set( E_PARSE_FAIL, lhs->pos, "cannot have more than one assigned argument of same name" );
				delete rhs;
				goto fail;
			}
			args.push_back( new stmt_fn_assn_arg_t( new stmt_simple_t( lhs ), rhs ) );
			done_assn_args[ lhs->data ] = lhs->pos;
		}
		else if( ph.peakt( 1 ) == TOK_TDOT ) { // perhaps a variadic
			va_arg = new stmt_simple_t( ph.peak() );
			ph.next(); ph.next();
		}
	} else {
		err::set( E_PARSE_FAIL, ph.peak()->pos, "failed to parse function def args" );
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
