/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Base.hpp"
#include "../Memory.hpp"

static size_t type_id()
{
	static size_t tid = 0;
	return tid++;
}

static size_t fn_id()
{
	static size_t fnid = 1;
	return fnid++;
}

var_base_t::var_base_t( const size_t & type, const size_t & idx, const size_t & ref, const bool is_type )
	: m_type( type ), m_idx( idx ), m_ref( ref ), m_is_type( is_type ) {}
var_base_t::~var_base_t() {}

var_nil_t::var_nil_t( const size_t & idx )
	: var_base_t( VT_NIL, idx, 1, false ) {}

var_base_t * var_nil_t::copy( const size_t & idx ) { return new var_nil_t( idx ); }
void var_nil_t::set( var_base_t * from ) {}

var_bool_t::var_bool_t( const bool val, const size_t & idx )
	: var_base_t( VT_BOOL, idx, 1, false ), m_val( val ) {}

var_base_t * var_bool_t::copy( const size_t & idx ) { return new var_bool_t( m_val, idx ); }
bool & var_bool_t::get() { return m_val; }
void var_bool_t::set( var_base_t * from )
{
	m_val = BOOL( from )->get();
}

var_int_t::var_int_t( const mpz_class & val, const size_t & idx )
	: var_base_t( VT_INT, idx, 1, false ), m_val( val ) {}

var_base_t * var_int_t::copy( const size_t & idx ) { return new var_int_t( m_val, idx ); }
mpz_class & var_int_t::get() { return m_val; }
void var_int_t::set( var_base_t * from )
{
	m_val = INT( from )->get();
}

var_flt_t::var_flt_t( const mpfr::mpreal & val, const size_t & idx )
	: var_base_t( VT_FLT, idx, 1, false ), m_val( val ) {}

var_base_t * var_flt_t::copy( const size_t & idx ) { return new var_flt_t( m_val, idx ); }
mpfr::mpreal & var_flt_t::get() { return m_val; }
void var_flt_t::set( var_base_t * from )
{
	m_val = FLT( from )->get();
}

var_str_t::var_str_t( const std::string & val, const size_t & idx )
	: var_base_t( VT_STR, idx, 1, false ), m_val( val ) {}

var_base_t * var_str_t::copy( const size_t & idx ) { return new var_str_t( m_val, idx ); }
std::string & var_str_t::get() { return m_val; }
void var_str_t::set( var_base_t * from )
{
	m_val = STR( from )->get();
}

void * var_base_t::operator new( size_t sz )
{
	return mem::alloc( sz );
}
void var_base_t::operator delete( void * ptr, size_t sz )
{
	mem::free( ptr, sz );
}

var_fn_t::var_fn_t( const std::string & kw_arg, const std::string & var_arg,
		    const std::vector< std::string > & args_order,
		    const std::unordered_map< std::string, var_base_t * > & args,
		    const fn_body_t & body, const bool is_native, const size_t & idx )
	: var_base_t( VT_FUNC, idx, 1, false ), m_fn_id( fn_id() ), m_kw_arg( kw_arg ),
	  m_var_arg( var_arg ), m_args_order( args_order ), m_args( args ), m_body( body ),
	  m_is_native( is_native ) {}

var_fn_t::~var_fn_t()
{
	for( auto & arg : m_args ) var_dref( arg.second );
}

var_base_t * var_fn_t::copy( const size_t & idx )
{
	for( auto & arg : m_args ) var_iref( arg.second );
	return new var_fn_t( m_kw_arg, m_var_arg, m_args_order, m_args, m_body, m_is_native, idx );
}

size_t & var_fn_t::fn_id() { return m_fn_id; }
std::string & var_fn_t::kw_arg() { return m_kw_arg; }
std::string & var_fn_t::var_arg() { return m_var_arg; }
std::vector< std::string > & var_fn_t::args_order() { return m_args_order; }
std::unordered_map< std::string, var_base_t * > & var_fn_t::args() { return m_args; }
fn_body_t & var_fn_t::body() { return m_body; }
bool var_fn_t::is_native() { return m_is_native; }

void var_fn_t::set( var_base_t * from )
{
	var_fn_t * fn = FN( from );
	for( auto & arg : m_args ) var_dref( arg.second );

	for( auto & arg : fn->m_args ) var_iref( arg.second );

	// no need to change fn id
	m_kw_arg = fn->m_kw_arg;
	m_var_arg = fn->m_var_arg;
	m_args_order = fn->m_args_order;
	m_args = fn->m_args;
	m_body = fn->m_body;
	m_is_native = fn->m_is_native;
}
