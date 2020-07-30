/*
	MIT License

	Copyright (c) 2020 Feral Language repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "VM/Memory.hpp"
#include "VM/Vars.hpp"

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

void vars_stack_t::push_loop()
{
	m_loops_from.push_back( m_top + 1 );
	inc_top( 1 );
}

void vars_stack_t::pop_loop()
{
	assert( m_loops_from.size() > 0 );
	if( m_top >= m_loops_from.back() ) {
		dec_top( m_top - m_loops_from.back() + 1 );
	}
	m_loops_from.pop_back();
}

void vars_stack_t::loop_continue()
{
	assert( m_loops_from.size() > 0 );
	if( m_top > m_loops_from.back() ) {
		dec_top( m_top - m_loops_from.back() );
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

vars_t::vars_t() : m_fn_stack( -1 )
{
	m_fn_vars[ 0 ] = new vars_stack_t;
}
vars_t::~vars_t()
{
	assert( m_fn_stack == 0 || m_fn_stack == -1 );
	delete m_fn_vars[ 0 ];
}

bool vars_t::exists( const std::string & name )
{
	return m_fn_vars[ m_fn_stack ]->exists( name );
}

var_base_t * vars_t::get( const std::string & name )
{
	var_base_t * res = m_fn_vars[ m_fn_stack ]->get( name );
	if( res == nullptr && m_fn_stack != 0 ) {
		res = m_fn_vars[ 0 ]->get( name );
	}
	return res;
}

void vars_t::blk_add( const size_t & count )
{
	m_fn_vars[ m_fn_stack ]->inc_top( count );
	for( auto & s : m_stash ) {
		m_fn_vars[ m_fn_stack ]->add( s.first, s.second, false );
	}
	m_stash.clear();
}

void vars_t::blk_rem( const size_t & count )
{
	m_fn_vars[ m_fn_stack ]->dec_top( count );
}

void vars_t::push_fn()
{
	++m_fn_stack;
	if( m_fn_stack == 0 ) return;
	m_fn_vars[ m_fn_stack ] = new vars_stack_t;
}
void vars_t::pop_fn()
{
	if( m_fn_stack == 0 ) return;
	delete m_fn_vars[ m_fn_stack ];
	m_fn_vars.erase( m_fn_stack );
	--m_fn_stack;
}

void vars_t::stash( const std::string & name, var_base_t * val, const bool & iref )
{
	if( iref ) var_iref( val );
	m_stash[ name ] = val;
}

void vars_t::unstash()
{
	for( auto & s : m_stash ) var_dref( s.second );
	m_stash.clear();
}

void vars_t::add( const std::string & name, var_base_t * val, const bool inc_ref )
{
	m_fn_vars[ m_fn_stack ]->add( name, val, inc_ref );
}

void vars_t::addm( const std::string & name, var_base_t * val, const bool inc_ref )
{
	m_fn_vars[ 0 ]->add( name, val, inc_ref );
}

void vars_t::rem( const std::string & name, const bool dec_ref )
{
	m_fn_vars[ m_fn_stack ]->rem( name, dec_ref );
}
