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

bool stmt_conditional_t::gen_code(bcode_t &bc) const
{
	std::vector<size_t> body_jmps;
	for(size_t i = 0; i < m_conds.size(); ++i) {
		size_t false_jmp_pos = 0;
		if(m_conds[i].condition) {
			m_conds[i].condition->gen_code(bc);
			false_jmp_pos = bc.size();
			bc.addsz(m_conds[i].idx, OP_JMPFPOP, 0);
		}

		m_conds[i].body->gen_code(bc);
		if(i < m_conds.size() - 1) {
			body_jmps.push_back(bc.size());
			bc.addsz(m_conds[i].idx, OP_JMP, 0);
		}
		if(m_conds[i].condition) {
			bc.updatesz(false_jmp_pos, bc.size());
		}
	}

	size_t jmp_to = bc.size();
	for(auto &jmp : body_jmps) {
		bc.updatesz(jmp, jmp_to);
	}

	return true;
}
