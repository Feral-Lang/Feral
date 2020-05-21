/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Compiler/CodeGen/Internal.hpp"

bool stmt_conditional_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	std::vector< size_t > body_jmps;
	for( size_t i = 0; i < m_conds.size(); ++i ) {
		size_t false_jmp_pos = 0;
		if( m_conds[ i ].condition ) {
			m_conds[ i ].condition->gen_code( bc );
			false_jmp_pos = bc.size();
			bc.addsz( m_conds[ i ].idx, OP_JMPFPOP, 0 );
		}

		m_conds[ i ].body->gen_code( bc );
		if( i < m_conds.size() - 1 ) {
			body_jmps.push_back( bc.size() );
			bc.addsz( m_conds[ i ].idx, OP_JMP, 0 );
		}
		if( m_conds[ i ].condition ) {
			bc.updatesz( false_jmp_pos, bc.size() );
		}
	}

	size_t jmp_to = bc.size();
	for( auto & jmp : body_jmps ) {
		bc.updatesz( jmp, jmp_to );
	}

	return true;
}
