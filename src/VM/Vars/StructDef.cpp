/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Base.hpp"

static size_t type_id()
{
	static size_t tid = _VT_LAST;
	return tid++;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////// VAR_STRUCT_DEF /////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_struct_def_t::var_struct_def_t( const std::vector< std::string > & attr_order,
				    const std::unordered_map< std::string, var_base_t * > & attrs,
				    const size_t & src_id, const size_t & idx )
	: var_base_t( type_id(), src_id, idx ), m_attr_order( attr_order ), m_attrs( attrs ) {}
var_struct_def_t::var_struct_def_t( const size_t & type_id, const std::vector< std::string > & attr_order,
				    const std::unordered_map< std::string, var_base_t * > & attrs,
				    const size_t & src_id, const size_t & idx )
	: var_base_t( type_id, src_id, idx ), m_attr_order( attr_order ), m_attrs( attrs ) {}

var_struct_def_t::~var_struct_def_t()
{
	for( auto & attr : m_attrs ) {
		var_dref( attr.second );
	}
}

var_base_t * var_struct_def_t::copy( const size_t & src_id, const size_t & idx )
{
	std::unordered_map< std::string, var_base_t * > attrs;
	for( auto & attr : m_attrs ) {
		attrs[ attr.first ] = attr.second->copy( src_id, idx );
	}
	return new var_struct_def_t( m_type, m_attr_order, attrs, src_id, idx );
}

void var_struct_def_t::set( var_base_t * from )
{
	var_struct_def_t * st = STRUCT_DEF( from );
	m_type = st->m_type;
	m_attr_order = st->m_attr_order;
	for( auto & attr : m_attrs ) {
		var_dref( attr.second );
	}
	for( auto & attr : st->m_attrs ) {
		var_iref( attr.second );
	}
	m_attrs = st->m_attrs;
}

const std::vector< std::string > & var_struct_def_t::attr_order() const { return m_attr_order; }
const std::unordered_map< std::string, var_base_t * > & var_struct_def_t::attrs() const { return m_attrs; }