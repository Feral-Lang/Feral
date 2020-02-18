/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Internal.hpp"

bool stmt_while_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	bc.addsz( idx(), OP_BLKA, 1 );

	// loop expression
	size_t begin_loop = bc.size();
	m_expr->gen_code( bc );

	size_t jmp_loop_out_loc = bc.size();
	bc.addsz( idx(), OP_JMPF, 0 );

	m_body->gen_code( bc );

	bc.addsz( idx(), OP_JMP, begin_loop );

	bc.updatesz( jmp_loop_out_loc, bc.size() );

	bc.addsz( idx(), OP_BLKR, 1 );
	return true;
}
