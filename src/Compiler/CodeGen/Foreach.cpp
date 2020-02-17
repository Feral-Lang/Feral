/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Internal.hpp"

bool stmt_foreach_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	bc.addsz( idx(), OP_BLKA, 1 );

	// create m_loop_var as nil
	bc.add( idx(), OP_LOAD );
	bc.adds( m_loop_var->pos, OP_LOAD, ODT_STR, m_loop_var->data );
	bc.addb( m_loop_var->pos, OP_CREATE, false );

	// loop expression
	size_t begin_loop = bc.size();
	m_expr->gen_code( bc );

	// magic function
	bc.adds( idx(), OP_LOAD, ODT_STR, "iter" );
	bc.add( idx(), OP_ATTR );
	bc.addb( idx(), OP_FNCL, false );

	// store in m_loop_var
	bc.adds( m_loop_var->pos, OP_LOAD, ODT_STR, m_loop_var->data );
	size_t jmp_loop_out_loc = bc.size();
	bc.addsz( idx(), OP_JMPN, 0 );
	bc.add( m_loop_var->pos, OP_STORE );

	m_body->gen_code( bc );

	bc.addsz( idx(), OP_JMP, begin_loop );

	bc.updatesz( jmp_loop_out_loc, bc.size() );

	bc.addsz( idx(), OP_BLKR, 1 );
	return true;
}
