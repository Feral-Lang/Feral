/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Compiler/CodeGen/Internal.hpp"

bool stmt_var_decl_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	for( auto & vd : m_decls ) {
		if( !vd->gen_code( bc ) ) return false;
	}
	return true;
}

bool stmt_var_decl_base_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	if( !m_rhs->gen_code( bc ) ) return false;
	if( m_in && !m_in->gen_code( bc ) ) return false;
	if( !m_lhs->gen_code( bc ) ) return false;

	bc.addb( idx(), OP_CREATE, m_in );
	return true;
}
