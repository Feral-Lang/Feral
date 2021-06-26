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

bool stmt_for_t::gen_code(bcode_t &bc) const
{
	bc.add(idx(), OP_PUSH_LOOP);

	if(m_init) m_init->gen_code(bc);

	size_t iter_jmp_loc = bc.size();

	if(m_cond) m_cond->gen_code(bc);
	size_t cond_fail_jmp_from = bc.size();
	if(m_cond) {
		// placeholder location
		bc.addsz(m_cond->idx(), OP_JMPFPOP, 0);
	}

	size_t body_begin = bc.size();
	m_body->gen_code(bc);
	size_t body_end = bc.size();

	size_t continue_jmp_loc = bc.size();

	if(m_incr) {
		m_incr->gen_code(bc);
		bc.add(m_incr->idx(), OP_ULOAD);
	}

	bc.addsz(idx(), OP_JMP, iter_jmp_loc);

	if(m_cond) {
		bc.updatesz(cond_fail_jmp_from, bc.size());
	}

	// pos where break goes
	size_t break_jmp_loc = bc.size();
	bc.add(idx(), OP_POP_LOOP);

	// update all continue and break calls
	for(size_t i = body_begin; i < body_end; ++i) {
		if(bc.at(i) == OP_CONTINUE && bc.get()[i].data.sz == 0)
			bc.updatesz(i, continue_jmp_loc);
		if(bc.at(i) == OP_BREAK && bc.get()[i].data.sz == 0) bc.updatesz(i, break_jmp_loc);
	}
	return true;
}
