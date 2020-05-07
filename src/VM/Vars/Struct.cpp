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
////////////////////////////////////////////////////////////// VAR_STRUCT //////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_struct_t::var_struct_t( const std::unordered_map< std::string, var_base_t * > & attrs,
			    const std::uintptr_t & struct_id, const size_t & src_id, const size_t & idx )
	: var_base_t( type_id< var_struct_t >(), src_id, idx, false, true ), m_attrs( attrs ), m_id( struct_id ) {}

var_struct_t::~var_struct_t()
{
	for( auto & attr : m_attrs ) {
		var_dref( attr.second );
	}
}

std::uintptr_t var_struct_t::typefn_id() const { return m_id; }

var_base_t * var_struct_t::copy( const size_t & src_id, const size_t & idx )
{
	std::unordered_map< std::string, var_base_t * > attrs;
	for( auto & attr : m_attrs ) {
		attrs[ attr.first ] = attr.second->copy( src_id, idx );
	}
	return new var_struct_t( attrs, m_id, src_id, idx );
}

void var_struct_t::set( var_base_t * from )
{
	var_struct_t * st = STRUCT( from );
	for( auto & attr : m_attrs ) {
		var_dref( attr.second );
	}
	for( auto & attr : st->m_attrs ) {
		var_iref( attr.second );
	}
	m_attrs = st->m_attrs;
	m_id = st->m_id;
}

bool var_struct_t::attr_exists( const std::string & name ) const
{
	return m_attrs.find( name ) != m_attrs.end();
}

void var_struct_t::attr_set( const std::string & name, var_base_t * val, const bool iref )
{
	if( m_attrs.find( name ) != m_attrs.end() ) {
		var_dref( m_attrs[ name ] );
	}
	if( iref ) var_iref( val );
	m_attrs[ name ] = val;
}

var_base_t * var_struct_t::attr_get( const std::string & name )
{
	if( m_attrs.find( name ) == m_attrs.end() ) return nullptr;
	return m_attrs[ name ];
}

const std::unordered_map< std::string, var_base_t * > & var_struct_t::attrs() const { return m_attrs; }