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
///////////////////////////////////////////////////////////// VAR_BOOL /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_bool_t::var_bool_t( const bool val, const size_t & src_id, const size_t & idx )
	: var_base_t( type_id< var_bool_t >(), src_id, idx, false, false ), m_val( val ) {}

var_base_t * var_bool_t::copy( const size_t & src_id, const size_t & idx ) { return new var_bool_t( m_val, src_id, idx ); }
bool & var_bool_t::get() { return m_val; }
void var_bool_t::set( var_base_t * from )
{
	m_val = BOOL( from )->get();
}
