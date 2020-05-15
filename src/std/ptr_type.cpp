/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "std/ptr_type.hpp"

var_ptr_t::var_ptr_t( var_base_t * val, const size_t & src_id, const size_t & idx )
	: var_base_t( type_id< var_ptr_t >(), src_id, idx, false, false ), m_val( val )
{ var_iref( m_val ); }
var_ptr_t::~var_ptr_t() { var_dref( m_val ); }

var_base_t * var_ptr_t::copy( const size_t & src_id, const size_t & idx )
{
	return new var_ptr_t( m_val, src_id, idx );
}
void var_ptr_t::set( var_base_t * from )
{
	var_dref( m_val );
	m_val = PTR( from )->m_val;
	var_iref( m_val );
}

void var_ptr_t::update( var_base_t * with )
{
	var_dref( m_val );
	m_val = with;
	var_iref( m_val );
}

var_base_t * var_ptr_t::get() { return m_val; }