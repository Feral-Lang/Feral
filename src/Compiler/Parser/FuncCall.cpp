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

Errors parse_fn_call_args(phelper_t &ph, stmt_base_t *&loc)
{
	// assn_args = kw args
	std::vector<const stmt_base_t *> args;
	std::vector<const stmt_fn_assn_arg_t *> assn_args;
	bool va_unpack	     = false;
	size_t va_unpack_pos = 0;
	stmt_base_t *expr    = nullptr;

	size_t idx = ph.peak()->pos;
begin:
	if(va_unpack) {
		err::set(E_PARSE_FAIL, va_unpack_pos,
			 "variadic unpack '...' can be on last argument only");
		goto fail;
	}
	if(ph.acceptd() && ph.peakt(1) == TOK_ASSN) {
		const lex::tok_t *lhs = ph.peak();
		if(ph.peakt() == TOK_IDEN) ph.sett(TOK_STR);
		stmt_base_t *rhs = nullptr;
		ph.next();
		ph.next();
		if(parse_expr_15(ph, rhs) != E_OK) {
			goto fail;
		}
		assn_args.push_back(new stmt_fn_assn_arg_t(new stmt_simple_t(lhs), rhs));
	} else if(parse_expr_15(ph, expr) == E_OK) {
		if(ph.accept(TOK_TDOT)) {
			va_unpack     = true;
			va_unpack_pos = ph.peak()->pos;
			ph.next();
		}
		args.push_back(expr);
	} else {
		err::set(E_PARSE_FAIL, ph.peak()->pos, "failed to parse function call args");
		goto fail;
	}

	if(ph.accept(TOK_COMMA)) {
		ph.next();
		goto begin;
	}

	loc = new stmt_fn_call_args_t(args, assn_args, va_unpack, idx);
	return E_OK;
fail:
	for(auto &arg : args) delete arg;
	return E_PARSE_FAIL;
}
