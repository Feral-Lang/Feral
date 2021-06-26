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

#include <cassert>

#include "Compiler/CodeGen/Internal.hpp"

// used for AND and OR operations - all locations from where to jump
static std::vector<size_t> jmp_locs;

bool stmt_expr_t::gen_code(bcode_t &bc) const
{
	size_t before_jmp_locs_count = jmp_locs.size();

	size_t or_jmp_pos = 0;
	if(m_or_blk) {
		or_jmp_pos = bc.size();
		bc.addsz(m_or_blk->idx(), OP_PUSH_JMP, 0);
		if(m_or_blk_var)
			bc.adds(m_or_blk->idx(), OP_PUSH_JMPN, ODT_STR, m_or_blk_var->data);
	}

	m_lhs->gen_code(bc);

	if(!m_oper) goto done;

	if(m_oper->type == TOK_LAND || m_oper->type == TOK_LOR) {
		jmp_locs.push_back(bc.size());
		bc.addsz(m_oper->pos, m_oper->type == TOK_LAND ? OP_JMPF : OP_JMPT, 0);
	}

	if(m_oper->type == TOK_ADD) bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_SUB)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_MUL)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_DIV)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_MOD)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);

	else if(m_oper->type == TOK_ADD_ASSN)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_SUB_ASSN)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_MUL_ASSN)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_DIV_ASSN)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_MOD_ASSN)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);

	else if(m_oper->type == TOK_POW)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_ROOT)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);

	else if(m_oper->type == TOK_LNOT)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);

	else if(m_oper->type == TOK_BAND)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_BOR)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_BNOT)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_BXOR)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);

	else if(m_oper->type == TOK_BAND_ASSN)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_BOR_ASSN)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_BNOT_ASSN)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_BXOR_ASSN)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);

	else if(m_oper->type == TOK_LSHIFT)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_RSHIFT)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);

	else if(m_oper->type == TOK_LSHIFT_ASSN)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_RSHIFT_ASSN)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);

	else if(m_oper->type == TOK_XINC)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_XDEC)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_INCX)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_DECX)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);

	else if(m_oper->type == TOK_UADD)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_USUB)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);

	else if(m_oper->type == TOK_EQ)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_LT)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_GT)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_LE)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_GE)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	else if(m_oper->type == TOK_NE)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);
	// subscript
	else if(m_oper->type == TOK_OPER_SUBS)
		bc.adds(m_oper->pos, OP_LOAD, ODT_STR, TokStrs[m_oper->type]);

	// dot is handled in the all operators section
	if(m_rhs && m_oper->type != TOK_DOT) {
		m_rhs->gen_code(bc);
	}

	if(m_oper->type == TOK_LAND || m_oper->type == TOK_LOR) {
		size_t curr_sz = bc.size();
		for(size_t i = before_jmp_locs_count; i < jmp_locs.size(); ++i) {
			bc.updatesz(jmp_locs[i], curr_sz);
		}
		size_t jmps_to_rem = jmp_locs.size() - before_jmp_locs_count;
		for(size_t i = 0; i < jmps_to_rem; ++i) jmp_locs.pop_back();
		goto done;
	}

	if(m_oper->type == TOK_ASSN) {
		bc.add(m_oper->pos, OP_STORE);
		goto done;
	} else if(m_oper->type == TOK_DOT) {
		assert(m_rhs->type() == GT_SIMPLE);
		bc.adds(m_oper->pos, OP_ATTR, ODT_STR,
			static_cast<const stmt_simple_t *>(m_rhs)->val()->data);
		goto done;
	} else if(m_oper->type == TOK_OPER_FN || m_oper->type == TOK_OPER_MEM_FN) {
		bc.adds(m_oper->pos, m_oper->type == TOK_OPER_FN ? OP_FNCL : OP_MEM_FNCL, ODT_STR,
			m_rhs ? fn_call_args.back() : "0");
		if(m_rhs) fn_call_args.pop_back();
		goto done;
	}

	// skip extras (functions and dummy)
	if(m_oper->type < TOK_OPER_FN || m_oper->type >= TOK_OPER_SUBS) {
		bc.adds(m_oper->pos, OP_MEM_FNCL, ODT_STR, m_rhs ? "00" : "0");
	}
done:
	if(m_or_blk) {
		bc.addsz(m_or_blk->idx(), OP_POP_JMP, 0);
		size_t bypass_or_blk_pos = bc.size();
		bc.addsz(m_or_blk->idx(), OP_JMP, 0);
		bc.updatesz(or_jmp_pos, bc.size());
		m_or_blk->gen_code(bc);
		bc.updatesz(bypass_or_blk_pos, bc.size());
	}

	if(m_with_cols) bc.add(idx(), OP_ULOAD);
	return true;
}
