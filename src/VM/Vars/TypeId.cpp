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
//////////////////////////////////////////////////////////// VAR_TYPEID ////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_typeid_t::var_typeid_t( const size_t & val, const size_t & src_id, const size_t & idx )
	: var_base_t( VT_TYPEID, src_id, idx, false, false ), m_val( val ) {}

var_base_t * var_typeid_t::copy( const size_t & src_id, const size_t & idx ) { return new var_typeid_t( m_val, src_id, idx ); }
void var_typeid_t::set( var_base_t * from )
{
	m_val = TYPEID( from )->get();
}
size_t & var_typeid_t::get() { return m_val; }
size_t var_typeid_t::id() const { return m_val; }