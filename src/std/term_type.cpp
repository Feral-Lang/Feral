/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "std/term_type.hpp"

var_term_t::var_term_t( const struct termios & term, const size_t & src_id, const size_t & idx )
	: var_base_t( type_id< var_term_t >(), src_id, idx, false, false ), m_term( term ) {}

var_base_t * var_term_t::copy( const size_t & src_id, const size_t & idx )
{
	return new var_term_t( m_term, src_id, idx );
}

void var_term_t::set( var_base_t * from )
{
	m_term = TERM( from )->get();
}