/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "VM/Vars/Base.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// VAR_FLT //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_flt_t::var_flt_t( const mpfr::mpreal & val, const size_t & src_id, const size_t & idx )
	: var_base_t( type_id< var_flt_t >(), src_id, idx, false, false ), m_val( val ) {}

var_base_t * var_flt_t::copy( const size_t & src_id, const size_t & idx ) { return new var_flt_t( m_val, src_id, idx ); }
mpfr::mpreal & var_flt_t::get() { return m_val; }
void var_flt_t::set( var_base_t * from )
{
	m_val = FLT( from )->get();
}
