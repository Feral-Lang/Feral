/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "std/vec_type.hpp"

var_vec_iterable_t::var_vec_iterable_t( var_vec_t * vec, const size_t & src_id, const size_t & idx )
	: var_base_t( type_id< var_vec_iterable_t >(), src_id, idx, false, false ), m_vec( vec ), m_curr( 0 )
{
	var_iref( m_vec );
}
var_vec_iterable_t::~var_vec_iterable_t() { var_dref( m_vec ); }

var_base_t * var_vec_iterable_t::copy( const size_t & src_id, const size_t & idx )
{
	return new var_vec_iterable_t( m_vec, src_id, idx );
}
void var_vec_iterable_t::set( var_base_t * from )
{
	var_dref( m_vec );
	m_vec = VEC_ITERABLE( from )->m_vec;
	var_iref( m_vec );
	m_curr = VEC_ITERABLE( from )->m_curr;
}

bool var_vec_iterable_t::next( var_base_t * & val )
{
	if( m_curr >= m_vec->get().size() ) return false;
	val = m_vec->get()[ m_curr++ ];
	return true;
}