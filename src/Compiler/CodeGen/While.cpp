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

bool stmt_while_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	bc.add( idx(), OP_PUSH_LOOP );

	// loop expression
	size_t begin_loop = bc.size();
	m_expr->gen_code( bc );

	size_t jmp_loop_out_loc = bc.size();
	bc.addsz( idx(), OP_JMPFPOP, 0 );

	size_t body_begin = bc.size();
	m_body->gen_code( bc );
	size_t body_end = bc.size();

	bc.addsz( idx(), OP_JMP, begin_loop );

	bc.updatesz( jmp_loop_out_loc, bc.size() );

	size_t break_jmp_loc = bc.size();
	bc.add( idx(), OP_POP_LOOP );

	// update all continue and break calls
	for( size_t i = body_begin; i < body_end; ++i ) {
		if( bc.at( i ) == OP_CONTINUE ) bc.updatesz( i, begin_loop );
		if( bc.at( i ) == OP_BREAK ) bc.updatesz( i, break_jmp_loc );
	}
	return true;
}
