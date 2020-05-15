/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "std/struct_type.hpp"
#include "std/map_type.hpp"

var_map_iterable_t::var_map_iterable_t( var_map_t * map, const size_t & src_id, const size_t & idx )
	: var_base_t( type_id< var_map_iterable_t >(), src_id, idx, false, false ), m_map( map ), m_curr( map->get().begin() )
{
	var_iref( m_map );
}
var_map_iterable_t::~var_map_iterable_t() { var_dref( m_map ); }

var_base_t * var_map_iterable_t::copy( const size_t & src_id, const size_t & idx )
{
	return new var_map_iterable_t( m_map, src_id, idx );
}
void var_map_iterable_t::set( var_base_t * from )
{
	var_dref( m_map );
	m_map = MAP_ITERABLE( from )->m_map;
	var_iref( m_map );
	m_curr = MAP_ITERABLE( from )->m_curr;
}

bool var_map_iterable_t::next( var_base_t * & val, const size_t & src_id, const size_t & idx )
{
	if( m_curr == m_map->get().end() ) return false;
	std::unordered_map< std::string, var_base_t * > attrs;
	var_iref( m_curr->second );
	attrs[ "0" ] = new var_str_t( m_curr->first, src_id, idx );
	attrs[ "1" ] = m_curr->second;
	val = make< var_struct_t >( type_id< var_map_iterable_t >(), attrs, nullptr );
	++m_curr;
	return true;
}