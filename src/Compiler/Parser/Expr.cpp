/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Internal.hpp"

Errors parse_expr_cols( phelper_t & ph, stmt_base_t * & loc )
{
	if( parse_expr( ph, loc ) != E_OK ) {
		goto fail;
	}
	if( !ph.accept( TOK_COLS ) ) {
		ph.fail( "expected semicolon at the end of expression, found: '%s'",
			 TokStrs[ ph.peakt() ] );
		goto fail;
	}
	ph.next();
	return E_OK;
fail:
	if( loc ) delete loc;
	return E_PARSE_FAIL;
}

Errors parse_expr( phelper_t & ph, stmt_base_t * & loc )
{
	return parse_expr_15( ph, loc );
}

// Left Associative
// ,
Errors parse_expr_15( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * lhs = nullptr, * rhs = nullptr;
	const lex::tok_t * oper = nullptr;

	size_t idx = ph.peak()->pos;
	size_t commas = 0;

	if( parse_expr_14( ph, rhs ) != E_OK ) {
		goto fail;
	}

	while( ph.accept( TOK_COMMA ) ) {
		++commas;
		idx = ph.peak()->pos;
		oper = ph.peak();
		ph.next();
		if( parse_expr_14( ph, lhs ) != E_OK ) {
			goto fail;
		}
		rhs = new stmt_expr_t( lhs, oper, rhs, idx );
		lhs = nullptr;
	}
	if( rhs->type() == GT_EXPR ) {
		static_cast< stmt_expr_t * >( rhs )->commas_set( commas );
	}
	loc = rhs;
	return E_OK;
fail:
	if( lhs ) delete lhs;
	if( rhs ) delete rhs;
	return E_PARSE_FAIL;
}
// Right Associative
// =
// += -=
// *= /= %=
// <<= >>=
// &= |= ^=
Errors parse_expr_14( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * lhs = nullptr, * rhs = nullptr;
	const lex::tok_t * oper = nullptr;

	size_t idx = ph.peak()->pos;

	if( parse_expr_13( ph, rhs ) != E_OK ) {
		goto fail;
	}

	while( ph.accept( TOK_ASSN, TOK_ADD_ASSN, TOK_SUB_ASSN ) ||
	       ph.accept( TOK_MUL_ASSN, TOK_DIV_ASSN, TOK_MOD_ASSN ) ||
	       ph.accept( TOK_LSHIFT_ASSN, TOK_RSHIFT_ASSN, TOK_BAND_ASSN ) ||
	       ph.accept( TOK_BOR_ASSN, TOK_BXOR_ASSN ) ) {
		idx = ph.peak()->pos;
		oper = ph.peak();
		ph.next();
		if( parse_expr_13( ph, lhs ) != E_OK ) {
			goto fail;
		}
		rhs = new stmt_expr_t( lhs, oper, rhs, idx );
		lhs = nullptr;
	}
	loc = rhs;
	return E_OK;
fail:
	if( lhs ) delete lhs;
	if( rhs ) delete rhs;
	return E_PARSE_FAIL;
}

// Left Associative
// ||
Errors parse_expr_13( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * lhs = nullptr, * rhs = nullptr;
	const lex::tok_t * oper = nullptr;

	size_t idx = ph.peak()->pos;

	if( parse_expr_12( ph, lhs ) != E_OK ) {
		goto fail;
	}

	while( ph.accept( TOK_OR ) ) {
		idx = ph.peak()->pos;
		oper = ph.peak();
		ph.next();
		if( parse_expr_12( ph, rhs ) != E_OK ) {
			goto fail;
		}
		lhs = new stmt_expr_t( lhs, oper, rhs, idx );
		rhs = nullptr;
	}
	loc = lhs;
	return E_OK;
fail:
	if( lhs ) delete lhs;
	if( rhs ) delete rhs;
	return E_PARSE_FAIL;
}

// Left Associative
// &&
Errors parse_expr_12( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * lhs = nullptr, * rhs = nullptr;
	const lex::tok_t * oper = nullptr;

	size_t idx = ph.peak()->pos;

	if( parse_expr_11( ph, lhs ) != E_OK ) {
		goto fail;
	}

	while( ph.accept( TOK_AND ) ) {
		idx = ph.peak()->pos;
		oper = ph.peak();
		ph.next();
		if( parse_expr_11( ph, rhs ) != E_OK ) {
			goto fail;
		}
		lhs = new stmt_expr_t( lhs, oper, rhs, idx );
		rhs = nullptr;
	}
	loc = lhs;
	return E_OK;
fail:
	if( lhs ) delete lhs;
	if( rhs ) delete rhs;
	return E_PARSE_FAIL;
}

// Left Associative
// |
Errors parse_expr_11( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * lhs = nullptr, * rhs = nullptr;
	const lex::tok_t * oper = nullptr;

	size_t idx = ph.peak()->pos;

	if( parse_expr_10( ph, lhs ) != E_OK ) {
		goto fail;
	}

	while( ph.accept( TOK_BOR ) ) {
		idx = ph.peak()->pos;
		oper = ph.peak();
		ph.next();
		if( parse_expr_10( ph, rhs ) != E_OK ) {
			goto fail;
		}
		lhs = new stmt_expr_t( lhs, oper, rhs, idx );
		rhs = nullptr;
	}
	loc = lhs;
	return E_OK;
fail:
	if( lhs ) delete lhs;
	if( rhs ) delete rhs;
	return E_PARSE_FAIL;
}

// Left Associative
// ^
Errors parse_expr_10( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * lhs = nullptr, * rhs = nullptr;
	const lex::tok_t * oper = nullptr;

	size_t idx = ph.peak()->pos;

	if( parse_expr_09( ph, lhs ) != E_OK ) {
		goto fail;
	}

	while( ph.accept( TOK_BXOR ) ) {
		idx = ph.peak()->pos;
		oper = ph.peak();
		ph.next();
		if( parse_expr_09( ph, rhs ) != E_OK ) {
			goto fail;
		}
		lhs = new stmt_expr_t( lhs, oper, rhs, idx );
		rhs = nullptr;
	}
	loc = lhs;
	return E_OK;
fail:
	if( lhs ) delete lhs;
	if( rhs ) delete rhs;
	return E_PARSE_FAIL;
}

// Left Associative
// &
Errors parse_expr_09( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * lhs = nullptr, * rhs = nullptr;
	const lex::tok_t * oper = nullptr;

	size_t idx = ph.peak()->pos;

	if( parse_expr_08( ph, lhs ) != E_OK ) {
		goto fail;
	}

	while( ph.accept( TOK_BAND ) ) {
		idx = ph.peak()->pos;
		oper = ph.peak();
		ph.next();
		if( parse_expr_08( ph, rhs ) != E_OK ) {
			goto fail;
		}
		lhs = new stmt_expr_t( lhs, oper, rhs, idx );
		rhs = nullptr;
	}
	loc = lhs;
	return E_OK;
fail:
	if( lhs ) delete lhs;
	if( rhs ) delete rhs;
	return E_PARSE_FAIL;
}

// Left Associative
// == !=
Errors parse_expr_08( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * lhs = nullptr, * rhs = nullptr;
	const lex::tok_t * oper = nullptr;

	size_t idx = ph.peak()->pos;

	if( parse_expr_07( ph, lhs ) != E_OK ) {
		goto fail;
	}

	while( ph.accept( TOK_EQ, TOK_NE ) ) {
		idx = ph.peak()->pos;
		oper = ph.peak();
		ph.next();
		if( parse_expr_07( ph, rhs ) != E_OK ) {
			goto fail;
		}
		lhs = new stmt_expr_t( lhs, oper, rhs, idx );
		rhs = nullptr;
	}
	loc = lhs;
	return E_OK;
fail:
	if( lhs ) delete lhs;
	if( rhs ) delete rhs;
	return E_PARSE_FAIL;
}

// Left Associative
// < <=
// > >=
Errors parse_expr_07( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * lhs = nullptr, * rhs = nullptr;
	const lex::tok_t * oper = nullptr;

	size_t idx = ph.peak()->pos;

	if( parse_expr_06( ph, lhs ) != E_OK ) {
		goto fail;
	}

	while( ph.accept( TOK_LT, TOK_LE ) ||
	       ph.accept( TOK_GT, TOK_GE ) ) {
		idx = ph.peak()->pos;
		oper = ph.peak();
		ph.next();
		if( parse_expr_06( ph, rhs ) != E_OK ) {
			goto fail;
		}
		lhs = new stmt_expr_t( lhs, oper, rhs, idx );
		rhs = nullptr;
	}
	loc = lhs;
	return E_OK;
fail:
	if( lhs ) delete lhs;
	if( rhs ) delete rhs;
	return E_PARSE_FAIL;
}

// Left Associative
// << >>
Errors parse_expr_06( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * lhs = nullptr, * rhs = nullptr;
	const lex::tok_t * oper = nullptr;

	size_t idx = ph.peak()->pos;

	if( parse_expr_05( ph, lhs ) != E_OK ) {
		goto fail;
	}

	while( ph.accept( TOK_LSHIFT, TOK_RSHIFT ) ) {
		idx = ph.peak()->pos;
		oper = ph.peak();
		ph.next();
		if( parse_expr_05( ph, rhs ) != E_OK ) {
			goto fail;
		}
		lhs = new stmt_expr_t( lhs, oper, rhs, idx );
		rhs = nullptr;
	}
	loc = lhs;
	return E_OK;
fail:
	if( lhs ) delete lhs;
	if( rhs ) delete rhs;
	return E_PARSE_FAIL;
}

// Left Associative
// + -
Errors parse_expr_05( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * lhs = nullptr, * rhs = nullptr;
	const lex::tok_t * oper = nullptr;

	size_t idx = ph.peak()->pos;

	if( parse_expr_04( ph, lhs ) != E_OK ) {
		goto fail;
	}

	while( ph.accept( TOK_ADD, TOK_SUB ) ) {
		idx = ph.peak()->pos;
		oper = ph.peak();
		ph.next();
		if( parse_expr_04( ph, rhs ) != E_OK ) {
			goto fail;
		}
		lhs = new stmt_expr_t( lhs, oper, rhs, idx );
		rhs = nullptr;
	}
	loc = lhs;
	return E_OK;
fail:
	if( lhs ) delete lhs;
	if( rhs ) delete rhs;
	return E_PARSE_FAIL;
}

// Left Associative
// / * % **
Errors parse_expr_04( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * lhs = nullptr, * rhs = nullptr;
	const lex::tok_t * oper = nullptr;

	size_t idx = ph.peak()->pos;

	if( parse_expr_03( ph, lhs ) != E_OK ) {
		goto fail;
	}

	while( ph.accept( TOK_DIV, TOK_MUL, TOK_MOD ) ||
	       ph.accept( TOK_POW ) ) {
		idx = ph.peak()->pos;
		oper = ph.peak();
		ph.next();
		if( parse_expr_03( ph, rhs ) != E_OK ) {
			goto fail;
		}
		lhs = new stmt_expr_t( lhs, oper, rhs, idx );
		rhs = nullptr;
	}
	loc = lhs;
	return E_OK;
fail:
	if( lhs ) delete lhs;
	if( rhs ) delete rhs;
	return E_PARSE_FAIL;
}

// Right Associative (single operand)
// ++ -- (pre)
// + - (unary)
// ! ~ (log/bit)
Errors parse_expr_03( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * lhs = nullptr;
	const lex::tok_t * oper = nullptr;
	std::vector< const lex::tok_t * > opers;

	size_t idx = ph.peak()->pos;

	// add operators in vector in reverse order
	while( ph.accept( TOK_XINC, TOK_XDEC ) ||
	       ph.accept( TOK_ADD, TOK_SUB ) ||
	       ph.accept( TOK_NOT, TOK_BNOT ) ) {
		if( ph.peakt() == TOK_XINC ) ph.sett( TOK_INCX );
		else if( ph.peakt() == TOK_XDEC ) ph.sett( TOK_DECX );
		else if( ph.peakt() == TOK_ADD ) ph.sett( TOK_UADD );
		else if( ph.peakt() == TOK_SUB ) ph.sett( TOK_USUB );

		opers.insert( opers.begin(), ph.peak() );
		ph.next();
	}
	if( parse_expr_02( ph, lhs ) != E_OK ) {
		goto fail;
	}
	for( auto & op : opers ) {
		lhs = new stmt_expr_t( lhs, op, nullptr, op->pos );
	}
	loc = lhs;
	return E_OK;
fail:
	if( lhs ) delete lhs;
	return E_PARSE_FAIL;
}

// Left Associative
// ++ -- (post)
Errors parse_expr_02( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * lhs = nullptr;
	const lex::tok_t * oper = nullptr;

	size_t idx = ph.peak()->pos;

	if( parse_expr_01( ph, lhs ) != E_OK ) {
		goto fail;
	}

	if( ph.accept( TOK_XINC, TOK_XDEC ) ) {
		idx = ph.peak()->pos;
		oper = ph.peak();
		ph.next();
		loc = new stmt_expr_t( lhs, oper, nullptr, idx );
		return E_OK;
	}

	loc = lhs;
	return E_OK;
fail:
	if( lhs ) delete lhs;
	return E_PARSE_FAIL;
}

// Left Associative
// x(FN_CALL_ARGS) y[EXPR]
// x.y
// '(' EXPR ')'
Errors parse_expr_01( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * lhs = nullptr, * rhs = nullptr;
	const lex::tok_t * oper = nullptr;

	size_t idx = ph.peak()->pos;

	if( ph.accept( TOK_LPAREN ) ) {
		idx = ph.peak()->pos;
		ph.next();
		if( parse_expr( ph, lhs ) != E_OK ) {
			goto fail;
		}
		if( !ph.accept( TOK_RPAREN ) ) {
			ph.fail( idx, "could not find ending parenthesis for expression" );
			goto fail;
		}
		ph.next();
		goto done;
	}

	if( ph.accept( TOK_IDEN ) && ph.peakt( 1 ) == TOK_LPAREN ) {
		ph.sett( TOK_STR ); // change IDEN -> STR for func calls
	}
	if( parse_term( ph, lhs ) != E_OK ) {
		goto fail;
	}

	while( ph.accept( TOK_LPAREN, TOK_LBRACK ) ||
	       ph.accept( TOK_DOT ) ) {
		idx = ph.peak()->pos;
		if( ph.accept( TOK_LPAREN ) ) {
			ph.sett( TOK_OPER_FN );
			oper = ph.peak();
			ph.next();
			if( !ph.accept( TOK_RPAREN ) && parse_fn_call_args( ph, rhs ) != E_OK ) {
				goto fail;
			}
			if( !ph.accept( TOK_RPAREN ) ) {
				ph.fail( idx, "missing closing parenthesis for function call" );
				goto fail;
			}
			ph.next();
		} else if( ph.accept( TOK_LBRACK ) ) {
			ph.sett( TOK_OPER_SUBS );
			oper = ph.peak();
			ph.next();
			if( !ph.accept( TOK_RBRACK ) && parse_expr( ph, rhs ) != E_OK ) {
				goto fail;
			}
			if( !ph.accept( TOK_RBRACK ) ) {
				ph.fail( idx, "missing closing bracket for subscript expression" );
				goto fail;
			}
			ph.next();
		} else if( ph.accept( TOK_DOT ) ) {
			oper = ph.peak();
			ph.next();
			if( parse_term( ph, rhs, true ) != E_OK ) {
				goto fail;
			}
		}
		lhs = new stmt_expr_t( lhs, oper, rhs, idx );
		rhs = nullptr;
	}
done:
	loc = lhs;
	return E_OK;
fail:
	if( lhs ) delete lhs;
	if( rhs ) delete rhs;
	return E_PARSE_FAIL;
}

// Data
Errors parse_term( phelper_t & ph, stmt_base_t * & loc, const bool make_const )
{
	if( ph.acceptd() ) {
		if( make_const && ph.peakt() == TOK_IDEN ) ph.sett( TOK_STR );
		loc = new stmt_simple_t( ph.peak() );
		ph.next();
	} else if( ph.accept( TOK_FN ) ) {
		if( parse_fn_decl( ph, loc ) != E_OK ) goto fail;
	} else {
		ph.fail( "invalid or extraneous token type '%s' received in expression", TokStrs[ ph.peakt() ] );
		goto fail;
	}
	return E_OK;
fail:
	return E_PARSE_FAIL;
}
