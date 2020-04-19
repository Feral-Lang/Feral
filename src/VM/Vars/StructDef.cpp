/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <algorithm>

#include "../VM.hpp"

#include "Base.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////// VAR_STRUCT_DEF /////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_struct_def_t::var_struct_def_t( const int & id, const std::vector< std::string > & attr_order,
				    const std::unordered_map< std::string, var_base_t * > & attrs,
				    const size_t & src_id, const size_t & idx )
	: var_base_t( VT_STRUCT_DEF, src_id, idx ), m_attr_order( attr_order ), m_attrs( attrs ),
	  m_id( id ) {}

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
	m_id = st->m_id;
	m_attr_order = st->m_attr_order;
	for( auto & attr : m_attrs ) {
		var_dref( attr.second );
	}
	for( auto & attr : st->m_attrs ) {
		var_iref( attr.second );
	}
	m_attrs = st->m_attrs;
}

var_base_t * var_struct_def_t::init( vm_state_t & vm, const std::vector< var_base_t * > & args,
				     const std::vector< fn_assn_arg_t > & assn_args,
				     const size_t & src_id, const size_t & idx )
{
	srcfile_t * src = vm.current_source_file();
	for( auto & aa : assn_args ) {
		if( std::find( m_attr_order.begin(), m_attr_order.end(), aa.name ) == m_attr_order.end() ) {
			src->fail( aa.idx, "no attribute named '%s' in the structure definition", aa.name.c_str() );
			return nullptr;
		}
	}
	std::unordered_map< std::string, var_base_t * > attrs;
	auto it = m_attr_order.begin();
	for( auto & arg : args ) {
		if( it == m_attr_order.end() ) {
			src->fail( arg->idx(), "provided more arguments than existing in structure definition" );
			goto fail;
		}
		if( m_attrs[ * it ]->type() != arg->type() ) {
			src->fail( arg->idx(), "expected type: %s, found: %s",
				   vm.type_name( m_attrs[ * it ]->type() ).c_str(),
				   vm.type_name( arg->type() ).c_str() );
			goto fail;
		}
		attrs[ * it ] = arg->copy( src_id, idx );
		++it;
	}

	for( auto & a_arg : assn_args ) {
		if( m_attrs.find( a_arg.name ) == m_attrs.end() ) {
			src->fail( a_arg.idx, "attribute %s does not exist in this strucutre",
				   a_arg.name.c_str() );
			goto fail;
		}
		if( m_attrs[ a_arg.name ]->type() != a_arg.val->type() ) {
			src->fail( a_arg.idx, "expected type: %s, found: %s",
				   vm.type_name( m_attrs[ a_arg.name ]->type() ).c_str(),
				   vm.type_name( a_arg.val->type() ).c_str() );
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

	return new var_struct_t( m_id, attrs, src_id, idx );
fail:
	for( auto attr : attrs ) {
		var_dref( attr.second );
	}
	return nullptr;
}

const std::vector< std::string > & var_struct_def_t::attr_order() const { return m_attr_order; }
const std::unordered_map< std::string, var_base_t * > & var_struct_def_t::attrs() const { return m_attrs; }
size_t var_struct_def_t::id() const { return m_id; }