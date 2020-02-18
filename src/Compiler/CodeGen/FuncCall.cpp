/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Internal.hpp"

bool stmt_fn_call_args_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	for( auto assn_arg = m_assn_args.rbegin(); assn_arg != m_assn_args.rend(); ++assn_arg ) {
		( * assn_arg )->gen_code( bc );
	}
	for( auto arg = m_args.rbegin(); arg != m_args.rend(); ++arg ) {
		( * arg )->gen_code( bc );
	}

	bc.addsz( idx(), OP_LOAD, m_assn_args.size() );
	bc.addsz( idx(), OP_LOAD, m_args.size() );
	return true;
}

bool stmt_fn_assn_arg_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	m_rhs->gen_code( bc );
	m_lhs->gen_code( bc );
	return true;
}
