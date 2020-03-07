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

vars_frame_t::vars_frame_t() {}
vars_frame_t::~vars_frame_t()
{
	for( auto & var : m_vars ) var_dref( var.second );
}

var_base_t * vars_frame_t::get( const std::string & name )
{
	if( m_vars.find( name ) == m_vars.end() ) return nullptr;
	return m_vars[ name ];
}

void vars_frame_t::add( const std::string & name, var_base_t * val, const bool inc_ref )
{
	if( m_vars.find( name ) != m_vars.end() ) {
		var_dref( m_vars[ name ] );
	}
	if( inc_ref ) var_iref( val );
	m_vars[ name ] = val;
}
void vars_frame_t::rem( const std::string & name, const bool dec_ref )
{
	if( m_vars.find( name ) == m_vars.end() ) return;
	if( dec_ref ) var_dref( m_vars[ name ] );
	m_vars.erase( name );
}

void * vars_frame_t::operator new( size_t sz )
{
	return mem::alloc( sz );
}
void vars_frame_t::operator delete( void * ptr, size_t sz )
{
	mem::free( ptr, sz );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

vars_stack_t::vars_stack_t()
	: m_top( 0 )
{
	m_stack.push_back( new vars_frame_t() );
}
vars_stack_t::~vars_stack_t()
{
	for( auto layer = m_stack.rbegin(); layer != m_stack.rend(); ++layer ) {
		delete * layer;
	}
}

bool vars_stack_t::exists( const std::string & name )
{
	return m_stack.back()->exists( name );
}

var_base_t * vars_stack_t::get( const std::string & name )
{
	for( auto layer = m_stack.rbegin(); layer != m_stack.rend(); ++layer ) {
		if( ( * layer )->exists( name ) ) {
			return ( * layer )->get( name );
		}
	}
	return nullptr;
}

void vars_stack_t::inc_top( const size_t & count )
{
	for( size_t i = 0; i < count; ++i ) {
		m_stack.push_back( new vars_frame_t() );
		++m_top;
	}
}
void vars_stack_t::dec_top( const size_t & count )
{
	if( m_top == 0 ) return;
	for( size_t i = 0; i < count && m_top > 0; ++i ) {
		delete m_stack.back();
		m_stack.pop_back();
		--m_top;
	}
}

void vars_stack_t::add( const std::string & name, var_base_t * val, const bool inc_ref )
{
	m_stack.back()->add( name, val, inc_ref );
}
void vars_stack_t::rem( const std::string & name, const bool dec_ref )
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

vars_t::vars_t() : m_fn_stack( { 0 } )
{
	m_fn_vars[ 0 ] = new vars_stack_t;
}
vars_t::~vars_t()
{
	assert( m_fn_stack.size() == 1 );
	delete m_fn_vars[ 0 ];
}

bool vars_t::exists( const std::string & name )
{
	return m_fn_vars[ m_fn_stack.back() ]->exists( name );
}

var_base_t * vars_t::get( const std::string & name )
{
	var_base_t * res = m_fn_vars[ m_fn_stack.back() ]->get( name );
	if( res == nullptr && m_fn_stack.back() != 0 ) {
		res = m_fn_vars[ 0 ]->get( name );
	}
	return res;
}

void vars_t::blk_add( const size_t & count )
{
	m_fn_vars[ m_fn_stack.back() ]->inc_top( count );
	for( auto & s : m_stash ) {
		m_fn_vars[ m_fn_stack.back() ]->add( s.first, s.second, true );
	}
	m_stash.clear();
}

void vars_t::blk_rem( const size_t & count )
{
	m_fn_vars[ m_fn_stack.back() ]->dec_top( count );
}

void vars_t::push_fn( const size_t & id )
{
	m_fn_stack.push_back( id );
	m_fn_vars[ id ] = new vars_stack_t;
}
void vars_t::pop_fn()
{
	assert( m_fn_stack.size() > 0 );
	delete m_fn_vars[ m_fn_stack.back() ];
	m_fn_vars.erase( m_fn_stack.back() );
	m_fn_stack.pop_back();
}

void vars_t::stash( const std::string & name, var_base_t * val )
{
	m_stash[ name ] = val;
}

void vars_t::unstash()
{
	m_stash.clear();
}

void vars_t::add( const std::string & name, var_base_t * val, const bool inc_ref )
{
	m_fn_vars[ m_fn_stack.back() ]->add( name, val, inc_ref );
}

void vars_t::rem( const std::string & name, const bool dec_ref )
{
	m_fn_vars[ m_fn_stack.back() ]->rem( name, dec_ref );
}
