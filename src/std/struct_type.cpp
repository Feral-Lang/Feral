/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <algorithm>

#include "std/struct_type.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////// VAR_STRUCT_DEF /////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_struct_def_t::var_struct_def_t( const std::uintptr_t & id, const std::vector< std::string > & attr_order,
				    const std::unordered_map< std::string, var_base_t * > & attrs,
				    const size_t & src_id, const size_t & idx )
	: var_base_t( type_id< var_struct_def_t >(), src_id, idx, true, true ), m_attr_order( attr_order ),
	  m_attrs( attrs ), m_id( id ) {}

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
	return new var_struct_def_t( m_id, m_attr_order, attrs, src_id, idx );
}

void var_struct_def_t::set( var_base_t * from )
{
	var_struct_def_t * st = STRUCT_DEF( from );
	m_attr_order = st->m_attr_order;
	for( auto & attr : m_attrs ) {
		var_dref( attr.second );
	}
	for( auto & attr : st->m_attrs ) {
		var_iref( attr.second );
	}
	m_attrs = st->m_attrs;
	m_id = st->m_id;
}

var_base_t * var_struct_def_t::call( vm_state_t & vm, const std::vector< var_base_t * > & args,
				     const std::vector< fn_assn_arg_t > & assn_args,
				     const std::unordered_map< std::string, size_t > & assn_args_loc,
				     const size_t & src_id, const size_t & idx )
{
	for( auto & aa : assn_args ) {
		if( std::find( m_attr_order.begin(), m_attr_order.end(), aa.name ) == m_attr_order.end() ) {
			vm.fail( aa.src_id, aa.idx, "no attribute named '%s' in the structure definition", aa.name.c_str() );
			return nullptr;
		}
	}
	std::unordered_map< std::string, var_base_t * > attrs;
	auto it = m_attr_order.begin();
	for( auto argit = args.begin() + 1; argit != args.end(); ++argit ) {
		auto & arg = * argit;
		if( it == m_attr_order.end() ) {
			vm.fail( arg->src_id(), arg->idx(), "provided more arguments than existing in structure definition" );
			goto fail;
		}
		if( m_attrs[ * it ]->type() != arg->type() ) {
			vm.fail( arg->src_id(), arg->idx(), "expected type: %s, found: %s",
				   vm.type_name( m_attrs[ * it ] ).c_str(),
				   vm.type_name( arg ).c_str() );
			goto fail;
		}
		attrs[ * it ] = arg->copy( src_id, idx );
		++it;
	}

	for( auto & a_arg : assn_args ) {
		if( m_attrs.find( a_arg.name ) == m_attrs.end() ) {
			vm.fail( a_arg.src_id, a_arg.idx, "attribute %s does not exist in this strucutre",
				 a_arg.name.c_str() );
			goto fail;
		}
		if( m_attrs[ a_arg.name ]->type() != a_arg.val->type() ) {
			vm.fail( a_arg.src_id, a_arg.idx, "expected type: %s, found: %s",
				 vm.type_name( m_attrs[ a_arg.name ] ).c_str(),
				 vm.type_name( a_arg.val ).c_str() );
			goto fail;
		}
		if( attrs.find( a_arg.name ) != attrs.end() ) {
			var_dref( attrs[ a_arg.name ] );
		}
		attrs[ a_arg.name ] = a_arg.val->copy( src_id, idx );
	}

	while( it < m_attr_order.end() ) {
		if( attrs.find( * it ) != attrs.end() ) { ++it; continue; }
		attrs[ * it ] = m_attrs[ * it ]->copy( src_id, idx );
		++it;
	}

	return new var_struct_t( m_id, attrs, this, src_id, idx );
fail:
	for( auto attr : attrs ) {
		var_dref( attr.second );
	}
	return nullptr;
}

bool var_struct_def_t::attr_exists( const std::string & name ) const
{
	return m_attrs.find( name ) != m_attrs.end();
}

void var_struct_def_t::attr_set( const std::string & name, var_base_t * val, const bool iref )
{
	if( m_attrs.find( name ) != m_attrs.end() ) {
		var_dref( m_attrs[ name ] );
	}
	if( iref ) var_iref( val );
	m_attrs[ name ] = val;
}

var_base_t * var_struct_def_t::attr_get( const std::string & name )
{
	if( m_attrs.find( name ) == m_attrs.end() ) return nullptr;
	return m_attrs[ name ];
}

const std::vector< std::string > & var_struct_def_t::attr_order() const { return m_attr_order; }
const std::unordered_map< std::string, var_base_t * > & var_struct_def_t::attrs() const { return m_attrs; }
std::uintptr_t var_struct_def_t::typefn_id() const { return m_id; }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////// VAR_STRUCT //////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_struct_t::var_struct_t( const std::uintptr_t & struct_id, const std::unordered_map< std::string, var_base_t * > & attrs,
			    var_struct_def_t * base, const size_t & src_id, const size_t & idx )
	: var_base_t( type_id< var_struct_t >(), src_id, idx, false, true ), m_attrs( attrs ), m_id( struct_id ), m_base( base )
{
	var_iref( m_base );
}

var_struct_t::~var_struct_t()
{
	for( auto & attr : m_attrs ) {
		var_dref( attr.second );
	}
	var_dref( m_base );
}

std::uintptr_t var_struct_t::typefn_id() const { return m_id; }

var_base_t * var_struct_t::copy( const size_t & src_id, const size_t & idx )
{
	std::unordered_map< std::string, var_base_t * > attrs;
	for( auto & attr : m_attrs ) {
		attrs[ attr.first ] = attr.second->copy( src_id, idx );
	}
	return new var_struct_t( m_id, attrs, m_base, src_id, idx );
}

void var_struct_t::set( var_base_t * from )
{
	var_struct_t * st = STRUCT( from );
	m_id = st->m_id;

	var_dref( m_base );
	var_iref( st->m_base );
	m_base = st->m_base;

	for( auto & attr : m_attrs ) {
		var_dref( attr.second );
	}
	for( auto & attr : st->m_attrs ) {
		var_iref( attr.second );
	}
	m_attrs = st->m_attrs;
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
	if( m_attrs.find( name ) == m_attrs.end() ) {
		return m_base ? m_base->attr_get( name ) : nullptr;
	}
	return m_attrs[ name ];
}

const std::unordered_map< std::string, var_base_t * > & var_struct_t::attrs() const { return m_attrs; }