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

#ifndef COMPILER_PARSER_INTERNAL_HPP
#define COMPILER_PARSER_INTERNAL_HPP

#include "ParseHelper.hpp" // for all the parse function sources
#include "Stmts.hpp"

Errors parse_block(phelper_t &ph, stmt_base_t *&loc, const bool with_brace = true);

Errors parse_expr_cols(phelper_t &ph, stmt_base_t *&loc);
Errors parse_expr_cols_or_rbrace(phelper_t &ph,
				 stmt_base_t *&loc); // consider expression done on right brace
Errors parse_expr(phelper_t &ph, stmt_base_t *&loc);
Errors parse_expr_16(phelper_t &ph, stmt_base_t *&loc);
Errors parse_expr_15(phelper_t &ph, stmt_base_t *&loc);
Errors parse_expr_14(phelper_t &ph, stmt_base_t *&loc);
Errors parse_expr_13(phelper_t &ph, stmt_base_t *&loc);
Errors parse_expr_12(phelper_t &ph, stmt_base_t *&loc);
Errors parse_expr_11(phelper_t &ph, stmt_base_t *&loc);
Errors parse_expr_10(phelper_t &ph, stmt_base_t *&loc);
Errors parse_expr_09(phelper_t &ph, stmt_base_t *&loc);
Errors parse_expr_08(phelper_t &ph, stmt_base_t *&loc);
Errors parse_expr_07(phelper_t &ph, stmt_base_t *&loc);
Errors parse_expr_06(phelper_t &ph, stmt_base_t *&loc);
Errors parse_expr_05(phelper_t &ph, stmt_base_t *&loc);
Errors parse_expr_04(phelper_t &ph, stmt_base_t *&loc);
Errors parse_expr_03(phelper_t &ph, stmt_base_t *&loc);
Errors parse_expr_02(phelper_t &ph, stmt_base_t *&loc);
Errors parse_expr_01(phelper_t &ph, stmt_base_t *&loc);
// make_const if true, changes type of token from IDEN to STR (useful for, say, after TOK_DOTs)
Errors parse_term(phelper_t &ph, stmt_base_t *&loc, const bool make_const = false);

Errors parse_var_decl(phelper_t &ph, stmt_base_t *&loc);
Errors parse_var_decl_base(phelper_t &ph, stmt_base_t *&loc);

Errors parse_fn_decl(phelper_t &ph, stmt_base_t *&loc);
Errors parse_fn_decl_args(phelper_t &ph, stmt_base_t *&loc);

Errors parse_fn_call_args(phelper_t &ph, stmt_base_t *&loc);

Errors parse_single_operand_stmt(phelper_t &ph, stmt_base_t *&loc);

Errors parse_conditional(phelper_t &ph, stmt_base_t *&loc);

Errors parse_for(phelper_t &ph, stmt_base_t *&loc);
Errors parse_foreach(phelper_t &ph, stmt_base_t *&loc);
Errors parse_while(phelper_t &ph, stmt_base_t *&loc);

#endif // COMPILER_PARSER_INTERNAL_HPP
