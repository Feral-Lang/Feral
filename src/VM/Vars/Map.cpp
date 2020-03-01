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
//////////////////////////////////////////////////////////// SOME EXTRAS ///////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::unordered_map< std::string, var_base_t * > * var_map_base()
{
	static std::unordered_map< std::string, var_base_t * > v;
	return & v;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// VAR_MAP //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_map_t::var_map_t( const std::unordered_map< std::string, var_base_t * > & val, const size_t & idx )
	: var_base_t( VT_MAP, idx, 1 ), m_val( val )
{
	fuse( VT_MAP, var_map_base() );
}
var_map_t::~var_map_t()
{
	for( auto & v : m_val ) var_dref( v.second );
}

var_base_t * var_map_t::copy( const size_t & idx )
{
	std::unordered_map< std::string, var_base_t * > new_map;
	for( auto & v : m_val ) {
		new_map[ v.first ] =  v.second->base_copy( idx );
	}
	return new var_map_t( new_map, idx );
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
