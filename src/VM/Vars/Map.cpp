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
///////////////////////////////////////////////////////////// VAR_MAP //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_map_t::var_map_t( const std::unordered_map< std::string, var_base_t * > & val, const size_t & src_id, const size_t & idx )
	: var_base_t( VT_MAP, src_id, idx, false, false ), m_val( val )
{
}
var_map_t::~var_map_t()
{
	for( auto & v : m_val ) var_dref( v.second );
}

var_base_t * var_map_t::copy( const size_t & src_id, const size_t & idx )
{
	std::unordered_map< std::string, var_base_t * > new_map;
	for( auto & v : m_val ) {
		new_map[ v.first ] =  v.second->copy( src_id, idx );
	}
	return new var_map_t( new_map, src_id, idx );
}
std::unordered_map< std::string, var_base_t * > & var_map_t::get() { return m_val; }
void var_map_t::set( var_base_t * from )
{
	for( auto & v : m_val ) {
		var_dref( v.second );
	}
	m_val.clear();
	for( auto & v : MAP( from )->m_val ) {
		var_iref( v.second );
	}
	m_val = MAP( from )->m_val;
}
