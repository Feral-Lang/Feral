/*
	MIT License

	Copyright (c) 2020 Feral Language repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
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
