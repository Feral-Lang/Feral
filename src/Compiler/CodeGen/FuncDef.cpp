/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Internal.hpp"

bool stmt_fn_def_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	size_t body_till_pos = bc.size();
	bc.addsz( idx(), OP_BODY_TILL, 0 );
	m_body->gen_code( bc );
	bc.updatesz( body_till_pos, bc.size() );

	if( m_args ) m_args->gen_code( bc );

	bc.add( idx(), OP_MKFN );
	return true;
}

bool stmt_fn_def_args_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	for( auto arg = m_args.rbegin(); arg != m_args.rend(); ++arg ) {
		( * arg )->gen_code( bc );
	}

	if( m_vaarg ) m_vaarg->gen_code( bc );
	if( m_kwarg ) m_kwarg->gen_code( bc );

	std::string arg_info;
	arg_info += m_kwarg ? "1" : "0";
	arg_info += m_vaarg ? "1" : "0";
	arg_info += std::to_string( m_args.size() );
	bc.adds( idx(), OP_LOAD, ODT_STR, arg_info );

	return true;
}
