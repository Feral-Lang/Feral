/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Base.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// VAR_INT //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_int_t::var_int_t( const mpz_class & val, const size_t & src_id, const size_t & idx )
	: var_base_t( VT_INT, src_id, idx ), m_val( val ) {}

var_base_t * var_int_t::copy( const size_t & src_id, const size_t & idx ) { return new var_int_t( m_val, src_id, idx ); }
mpz_class & var_int_t::get() { return m_val; }
void var_int_t::set( var_base_t * from )
{
	m_val = INT( from )->get();
}
