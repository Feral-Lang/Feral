/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "InbuiltTypes.hpp"

vartype_base_t::vartype_base_t( const size_t & type, const size_t & idx )
	: var_base_t( type, idx, 1, true ) {}
vartype_base_t::vartype_base_t( const size_t & type, const std::unordered_map< std::string, var_base_t * > & attrs, 
				const size_t & idx )
	: var_base_t( type, idx, 1, true ), m_attrs( attrs )
{
	for( auto & a : m_attrs ) var_iref( a.second );
}
vartype_base_t::~vartype_base_t()
{
	for( auto & a : m_attrs ) var_dref( a.second );
}
void vartype_base_t::add_attr( const std::string & name, var_base_t * val )
{
	if( m_attrs.find( name ) != m_attrs.end() ) {
		var_dref( m_attrs[ name ] );
	}
	var_iref( val );
	m_attrs[ name ] = val;
}
var_base_t * vartype_base_t::get_attr( const std::string & name )
{
	if( m_attrs.find( name ) == m_attrs.end() ) return nullptr;
	return m_attrs[ name ];
}
bool vartype_base_t::attr_exists( const std::string & name ) { return m_attrs.find( name ) != m_attrs.end(); }

vartype_nil_t::vartype_nil_t( const size_t & idx )
	: vartype_base_t( VT_NIL, idx ) {}
vartype_nil_t::~vartype_nil_t() {}

vartype_bool_t::vartype_bool_t( const size_t & idx )
	: vartype_base_t( VT_BOOL, idx ) {}
vartype_bool_t::~vartype_bool_t() {}

vartype_int_t::vartype_int_t( const size_t & idx )
	: vartype_base_t( VT_INT, idx ) {}
vartype_int_t::~vartype_int_t() {}

vartype_flt_t::vartype_flt_t( const size_t & idx )
	: vartype_base_t( VT_FLT, idx ) {}
vartype_flt_t::~vartype_flt_t() {}

vartype_str_t::vartype_str_t( const size_t & idx )
	: vartype_base_t( VT_STR, idx ) {}
vartype_str_t::~vartype_str_t() {}

vartype_type_t::vartype_type_t( const std::vector< std::string > & fields_pos,
				std::unordered_map< std::string, var_base_t * > & attrs,
				const size_t & idx )
	: vartype_base_t( VT_TYPE, attrs, idx ), m_fields_pos( fields_pos ) {}
vartype_type_t::~vartype_type_t() {}
void vartype_type_t::add_attr( const std::string & name, var_base_t * val )
{
	if( m_attrs.find( name ) != m_attrs.end() ) {
		var_dref( m_attrs[ name ] );
	}
	m_fields_pos.push_back( name );
	var_iref( val );
	m_attrs[ name ] = val;
}
size_t vartype_type_t::get_attr_pos( const std::string & name )
{
	for( size_t i = 0; i < m_fields_pos.size(); ++i ) {
		if( m_fields_pos[ i ] == name ) return i;
	}
	return m_fields_pos.size();
}
