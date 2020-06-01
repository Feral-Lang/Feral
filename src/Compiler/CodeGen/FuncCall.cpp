/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Compiler/CodeGen/Internal.hpp"

std::vector< std::string > fn_call_args;

bool stmt_fn_call_args_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	fn_call_args.emplace_back();

	for( auto assn_arg = m_assn_args.rbegin(); assn_arg != m_assn_args.rend(); ++assn_arg ) {
		( * assn_arg )->gen_code( bc );
	}
	for( auto arg = m_args.rbegin(); arg != m_args.rend(); ++arg ) {
		( * arg )->gen_code( bc );
	}

	fn_call_args.back() += m_va_unpack ? '1' : '0';
	fn_call_args.back() += std::string( m_args.size(), '0' );
	fn_call_args.back() += std::string( m_assn_args.size(), '1' );
	return true;
}

bool stmt_fn_assn_arg_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	m_rhs->gen_code( bc );
	m_lhs->gen_code( bc );
	return true;
}
