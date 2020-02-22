/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Vars.hpp"

vars_t::vars_t() {}
vars_t::~vars_t()
{
	for( auto & var : m_vars ) var_dref( var.second );
}

var_base_t * vars_t::get( const std::string & name )
{
	if( m_vars.find( name ) == m_vars.end() ) return nullptr;
	return m_vars[ name ];
}

void vars_t::add( const std::string & name, var_base_t * val, const bool inc_ref )
{
	if( m_vars.find( name ) != m_vars.end() ) {
		var_dref( m_vars[ name ] );
	}
	if( inc_ref ) var_iref( val );
	m_vars[ name ] = val;
}
void vars_t::rem( const std::string & name, const bool dec_ref )
{
	if( m_vars.find( name ) == m_vars.end() ) return;
	if( dec_ref ) var_dref( m_vars[ name ] );
	m_vars.erase( name );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_stack_t::var_stack_t()
	: m_top( 0 )
{
	m_stack.push_back( {} );
}
var_stack_t::~var_stack_t() {}

bool var_stack_t::exists( const std::string & name, const bool all_scopes )
{
	for( auto layer = m_stack.rbegin(); layer != m_stack.rend(); ++layer ) {
		if( layer->exists( name ) ) {
			return true;
		}
		if( !all_scopes ) break;
	}
	return false;
}

var_base_t * var_stack_t::get( const std::string & name )
{
	for( auto layer = m_stack.rbegin(); layer != m_stack.rend(); ++layer ) {
		if( layer->exists( name ) ) {
			return layer->get( name );
		}
	}
	return nullptr;
}

void var_stack_t::inc_top( const size_t & count )
{
	for( size_t i = 0; i < count; ++i ) {
		if( m_top + 1 < m_stack.size() - 1 ) {
			m_stack.push_back( {} );
		}
		++m_top;
	}
}
void var_stack_t::dec_top( const size_t & count )
{
	if( m_top == 0 ) return;
	for( size_t i = 0; i < count && m_top > 0; ++i ) {
		while( m_top < m_stack.size() - 1 ) {
			m_stack.pop_back();
		}
		--m_top;
	}
}

void var_stack_t::add( const std::string & name, var_base_t * val, const bool inc_ref )
{
	m_stack.back().add( name, val, inc_ref );
}
// adds variables to next value of top
void var_stack_t::add_no_inc()
{
	m_stack.push_back( {} );
}
void var_stack_t::rem( const std::string & name, const bool dec_ref )
{
	for( auto layer = m_stack.rbegin(); layer != m_stack.rend(); ++layer ) {
		if( layer->exists( name ) ) {
			layer->rem( name, dec_ref );
			return;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_srcfile_t::var_srcfile_t() {}
var_srcfile_t::~var_srcfile_t() {}

bool var_srcfile_t::exists( const std::string & name, const bool in_fn, const bool all_scopes )
{
	bool res = false;
	if( in_fn ) {
		res = m_fn_vars[ m_curr_fn_stack.back() ].exists( name, all_scopes );
		if( !res && all_scopes ) res = m_src_vars.exists( name );
		return res;
	} else {
		res = m_src_vars.exists( name );
	}
	return res;

	return m_fn_vars[ m_curr_fn_stack.back() ].exists( name, all_scopes ) ||
	       ( !in_fn && m_src_vars.exists( name ) );
}

var_base_t * var_srcfile_t::get( const std::string & name )
{
	var_base_t * res = m_fn_vars[ m_curr_fn_stack.back() ].get( name );
	if( !res ) res = m_src_vars.get( name );
	return res;
}

void var_srcfile_t::add( const std::string & name, var_base_t * val, const bool in_fn, const bool inc_ref )
{
	if( in_fn ) m_fn_vars[ m_curr_fn_stack.back() ].add( name, val, inc_ref );
	else m_src_vars.add( name, val, inc_ref );
}

void var_srcfile_t::rem( const std::string & name, const bool in_fn, const bool dec_ref )
{
	if( in_fn ) m_fn_vars[ m_curr_fn_stack.back() ].rem( name, dec_ref );
	else m_src_vars.rem( name, dec_ref );
}
