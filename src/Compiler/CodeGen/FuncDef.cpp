/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Internal.hpp"

static std::vector< std::string > fn_args;

bool stmt_fn_def_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	size_t body_till_pos = bc.size();
	bc.addsz( idx(), OP_BODY_TILL, 0 );
	if( !m_body->gen_code( bc ) ) return false;
	bc.updatesz( body_till_pos, bc.size() );

	if( m_args ) {
		if( !m_args->gen_code( bc ) ) return false;
	}

	bc.adds( idx(), OP_MKFN, ODT_STR, fn_args.back() );
	fn_args.pop_back();
	return true;
}

bool stmt_fn_def_args_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	std::string arg_info;
	arg_info += m_kwarg ? "1" : "0";
	arg_info += m_vaarg ? "1" : "0";

	for( auto arg = m_args.rbegin(); arg != m_args.rend(); ++arg ) {
		if( !( * arg )->gen_code( bc ) ) return false;
	}

	for( auto & arg : m_args ) {
		arg_info += arg->type() == GT_FN_ASSN_ARG ? "1" : "0";
	}

	if( m_vaarg ) {
		if( !m_vaarg->gen_code( bc ) ) return false;
	}
	if( m_kwarg ) {
		if( !m_kwarg->gen_code( bc ) ) return false;
	}

	fn_args.push_back( arg_info );
	return true;
}
