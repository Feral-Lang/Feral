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

bool stmt_var_decl_t::gen_code(bcode_t &bc) const
{
	for(auto &vd : m_decls) {
		if(!vd->gen_code(bc)) return false;
	}
	return true;
}

bool stmt_var_decl_base_t::gen_code(bcode_t &bc) const
{
	if(!m_rhs->gen_code(bc)) return false;
	if(m_in && !m_in->gen_code(bc)) return false;
	if(!m_lhs->gen_code(bc)) return false;

	bc.addb(idx(), OP_CREATE, m_in);
	return true;
}
