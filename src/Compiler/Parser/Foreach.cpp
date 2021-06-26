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

Errors parse_foreach(phelper_t &ph, stmt_base_t *&loc)
{
	const lex::tok_t *loop_var = nullptr;
	stmt_base_t *expr	   = nullptr;
	stmt_base_t *body	   = nullptr;

	size_t idx = ph.peak()->pos;
	ph.next();
	if(!ph.accept(TOK_IDEN)) {
		err::set(E_PARSE_FAIL, ph.peak()->pos, "expected identifier for loop variable");
		goto fail;
	}
	loop_var = ph.peak();
	ph.next();

	if(!ph.accept(TOK_IN)) {
		err::set(E_PARSE_FAIL, ph.peak()->pos,
			 "expected token 'in' for foreach loop after identifier");
		goto fail;
	}
	ph.next();

	if(parse_expr_01(ph, expr) != E_OK) {
		goto fail;
	}

	if(!ph.accept(TOK_LBRACE)) {
		err::set(E_PARSE_FAIL, ph.peak()->pos,
			 "expected left brace to begin body of loop, found: '%s'",
			 TokStrs[ph.peakt()]);
		goto fail;
	}
	if(parse_block(ph, body) != E_OK) {
		goto fail;
	}
done:
	loc = new stmt_foreach_t(loop_var, expr, body, idx);
	return E_OK;
fail:
	if(expr) delete expr;
	if(body) delete body;
	return E_PARSE_FAIL;
}
