/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Compiler/CodeGen/Internal.hpp"

bool stmt_foreach_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	bc.add( idx(), OP_PUSH_LOOP );

	// create __<loop_var> from expression
	m_expr->gen_code( bc );
	bc.adds( m_expr->idx(), OP_LOAD, ODT_STR, "__" + m_loop_var->data );
	bc.addb( m_expr->idx(), OP_CREATE, false );

	// let <loop_var> = __<loop_var>.next()
	bc.adds( m_expr->idx(), OP_LOAD, ODT_IDEN, "__" + m_loop_var->data );
	bc.adds( m_expr->idx(), OP_LOAD, ODT_STR, "next" );
	bc.adds( m_expr->idx(), OP_MEM_FNCL, ODT_STR, "" );
	// will be set later
	size_t jmp_loop_out_loc1 = bc.size();
	bc.addsz( m_loop_var->pos, OP_JMPN, 0 );
	bc.adds( m_loop_var->pos, OP_LOAD, ODT_STR, m_loop_var->data );
	bc.addb( m_loop_var->pos, OP_CREATE, false );

	// now comes the body of the loop
	size_t body_begin = bc.size();
	m_body->gen_code( bc );
	size_t body_end = bc.size();

	size_t continue_jmp_pos = bc.size();
	// next element please; if next returns nil, exit loop
	bc.adds( m_loop_var->pos, OP_LOAD, ODT_IDEN, "__" + m_loop_var->data );
	bc.adds( m_loop_var->pos, OP_LOAD, ODT_STR, "next" );
	bc.adds( m_loop_var->pos, OP_MEM_FNCL, ODT_STR, "" );
	// will be set later
	size_t jmp_loop_out_loc2 = bc.size();
	bc.addsz( m_loop_var->pos, OP_JMPN, 0 );
	bc.adds( m_loop_var->pos, OP_LOAD, ODT_IDEN, m_loop_var->data );
	bc.add( m_loop_var->pos, OP_STORE );
	bc.add( m_loop_var->pos, OP_ULOAD );
	bc.addsz( idx(), OP_JMP, body_begin );

	// pos where break goes
	size_t break_jmp_loc = bc.size();
	bc.add( idx(), OP_POP_LOOP );

	bc.updatesz( jmp_loop_out_loc1, break_jmp_loc );
	bc.updatesz( jmp_loop_out_loc2, break_jmp_loc );

	// update all continue and break calls
	for( size_t i = body_begin; i < body_end; ++i ) {
		if( bc.at( i ) == OP_CONTINUE && bc.get()[ i ].data.sz == 0 ) bc.updatesz( i, continue_jmp_pos );
		if( bc.at( i ) == OP_BREAK && bc.get()[ i ].data.sz == 0 ) bc.updatesz( i, break_jmp_loc );
	}
	return true;
}
