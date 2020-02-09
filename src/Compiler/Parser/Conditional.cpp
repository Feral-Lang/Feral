/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Internal.hpp"

Errors parse_conditional( phelper_t & ph, stmt_base_t * & loc )
{
	std::vector< conditional_t > conds;
	conditional_t cond;
	bool got_else = false;

	const lex::tok_t * tok = nullptr;

	cond.idx = ph.peak()->pos;

_if_elif:
	if( got_else ) {
		ph.fail( "cannot have an else if block after else block for a condtion" );
		goto fail;
	}
	tok = ph.peak();
	ph.next();
	if( parse_expr_14( ph, cond.condition ) != E_OK ) {
		goto fail;
	}
	goto block;
_else:
	if( got_else ) {
		ph.fail( "cannot have more than one else block for a condtion" );
		goto fail;
	}
	tok = ph.peak();
	ph.next();
	got_else = true;
block:
	if( !ph.accept( TOK_LBRACE ) ) {
		ph.fail( "expected block for statement '%s'", TokStrs[ tok->type ] );
		goto fail;
	}
	if( parse_block( ph, cond.body ) != E_OK ) {
		goto fail;
	}

	conds.push_back( cond );
	cond.condition = nullptr;
	cond.body = nullptr;

	cond.idx = ph.peak()->pos;
	if( ph.accept( TOK_ELIF ) ) {
		goto _if_elif;
	} else if( ph.accept( TOK_ELSE ) ) {
		goto _else;
	}

done:
	loc = new stmt_conditional_t( conds, conds[ 0 ].idx );
	return E_OK;
fail:
	for( auto & c : conds ) {
		if( c.condition ) delete c.condition;
		delete c.body;
	}
	return E_PARSE_FAIL;
}
