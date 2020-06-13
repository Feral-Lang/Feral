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

Errors parse_while( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * expr = nullptr;
	stmt_base_t * body = nullptr;

	size_t idx = ph.peak()->pos;
	ph.next();

	if( parse_expr_15( ph, expr ) != E_OK ) {
		goto fail;
	}
	if( parse_block( ph, body ) != E_OK ) {
		goto fail;
	}

	loc = new stmt_while_t( expr, body, idx );
	return E_OK;
fail:
	if( expr ) delete expr;
	if( body ) delete body;
	return E_PARSE_FAIL;
}
