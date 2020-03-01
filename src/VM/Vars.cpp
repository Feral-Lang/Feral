/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Vars.hpp"
#include "Memory.hpp"

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

void * vars_t::operator new( size_t sz )
{
	return mem::alloc( sz );
}
void vars_t::operator delete( void * ptr, size_t sz )
{
	mem::free( ptr, sz );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_stack_t::var_stack_t()
	: m_size( 1 )
{
	m_stack.push_back( new vars_t() );
}
var_stack_t::~var_stack_t()
{
	for( auto layer = m_stack.rbegin(); layer != m_stack.rend(); ++layer ) {
		delete * layer;
	}
}

bool var_stack_t::exists( const std::string & name, const bool all_scopes )
{
	for( auto layer = m_stack.rbegin(); layer != m_stack.rend(); ++layer ) {
		if( ( * layer )->exists( name ) ) {
			return true;
		}
		if( !all_scopes ) break;
	}
	return false;
}

var_base_t * var_stack_t::get( const std::string & name )
{
	for( auto layer = m_stack.rbegin(); layer != m_stack.rend(); ++layer ) {
		if( ( * layer )->exists( name ) ) {
			return ( * layer )->get( name );
		}
	}
	return nullptr;
}

void var_stack_t::inc_top( const size_t & count )
{
	for( size_t i = 0; i < count; ++i ) {
		if( m_stack.size() == m_size ) m_stack.push_back( new vars_t() );
		++m_size;
	}
}
void var_stack_t::dec_top( const size_t & count )
{
	if( m_size == 0 ) return;
	for( size_t i = 0; i < count && m_size > 0; ++i ) {
		delete m_stack.back();
		m_stack.pop_back();
		--m_size;
	}
}

void var_stack_t::add( const std::string & name, var_base_t * val, const bool inc_ref )
{
	m_stack.back()->add( name, val, inc_ref );
}
void var_stack_t::rem( const std::string & name, const bool dec_ref )
{
	for( auto layer = m_stack.rbegin(); layer != m_stack.rend(); ++layer ) {
		if( ( * layer )->exists( name ) ) {
			( * layer )->rem( name, dec_ref );
			return;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

srcfile_vars_t::srcfile_vars_t() {}
srcfile_vars_t::~srcfile_vars_t() {}

bool srcfile_vars_t::exists( const std::string & name, const bool in_fn, const bool all_scopes )
{
	bool res = false;
	if( in_fn && m_curr_fn_stack.size() > 0 ) {
		res = m_fn_vars[ m_curr_fn_stack.back() ]->exists( name, all_scopes );
		if( !res && all_scopes ) res = m_src_vars.exists( name, all_scopes );
		return res;
	} else {
		res = m_src_vars.exists( name, all_scopes );
	}
	return res;
}

var_base_t * srcfile_vars_t::get( const std::string & name )
{
	var_base_t * res = nullptr;
	if( m_curr_fn_stack.size() > 0 ) res = m_fn_vars[ m_curr_fn_stack.back() ]->get( name );
	if( !res ) res = m_src_vars.get( name );
	return res;
}

void srcfile_vars_t::blk_add( const size_t & count, const bool in_fn )
{
	if( in_fn && m_curr_fn_stack.size() > 0 ) {
		m_fn_vars[ m_curr_fn_stack.back() ]->inc_top( count );
		for( auto & s : m_stash ) {
			m_fn_vars[ m_curr_fn_stack.back() ]->add( s.first, s.second, true );
		}
	} else {
		m_src_vars.inc_top( count );
		for( auto & s : m_stash ) {
			m_src_vars.add( s.first, s.second, true );
		}
	}
	m_stash.clear();
}

void srcfile_vars_t::blk_rem( const size_t & count, const bool in_fn )
{
	if( in_fn && m_curr_fn_stack.size() > 0 ) m_fn_vars[ m_curr_fn_stack.back() ]->dec_top( count );
	else m_src_vars.dec_top( count );
}

void srcfile_vars_t::push_fn_id( const size_t & id )
{
	m_curr_fn_stack.push_back( id );
	m_fn_vars[ id ] = new var_stack_t;
}
void srcfile_vars_t::pop_fn_id()
{
	assert( m_curr_fn_stack.size() > 0 );
	delete m_fn_vars[ m_curr_fn_stack.back() ];
	m_fn_vars.erase( m_curr_fn_stack.back() );
	m_curr_fn_stack.pop_back();
}

void srcfile_vars_t::stash( const std::string & name, var_base_t * val )
{
	m_stash[ name ] = val;
}

void srcfile_vars_t::unstash()
{
	m_stash.clear();
}

void srcfile_vars_t::add( const std::string & name, var_base_t * val, const bool in_fn, const bool inc_ref )
{
	if( in_fn && m_curr_fn_stack.size() > 0 ) m_fn_vars[ m_curr_fn_stack.back() ]->add( name, val, inc_ref );
	else m_src_vars.add( name, val, inc_ref );
}

void srcfile_vars_t::rem( const std::string & name, const bool in_fn, const bool dec_ref )
{
	if( in_fn && m_curr_fn_stack.size() > 0 ) m_fn_vars[ m_curr_fn_stack.back() ]->rem( name, dec_ref );
	else m_src_vars.rem( name, dec_ref );
}
