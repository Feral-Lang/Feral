/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Internal.hpp"

bool stmt_for_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	bc.addsz( idx(), OP_BLKA, 1 );

	if( m_init ) m_init->gen_code( bc );

	size_t iter_jmp_loc = bc.size();

	if( m_cond ) m_cond->gen_code( bc );
	size_t cond_fail_jmp_from = bc.size();
	if( m_cond ) {
		// placeholder location
		bc.addsz( m_cond->idx(), OP_JMPF, 0 );
	}

	size_t body_begin = bc.size();
	m_body->gen_code( bc );
	size_t body_end = bc.size();

	size_t continue_jmp_loc = bc.size();

	if( m_incr ) m_incr->gen_code( bc );

	bc.addsz( idx(), OP_JMP, iter_jmp_loc );

	if( m_cond ) {
		bc.updatesz( cond_fail_jmp_from, bc.size() );
	}

	// pos where break goes
	size_t break_jmp_loc = bc.size();
	bc.addsz( idx(), OP_BLKR, 1 );

	// update all continue and break calls
	for( size_t i = body_begin; i < body_end; ++i ) {
		if( bc.at( i ) == OP_CONTINUE ) bc.updatesz( i, continue_jmp_loc );
		if( bc.at( i ) == OP_BREAK ) bc.updatesz( i, break_jmp_loc );
	}
	return true;
}
