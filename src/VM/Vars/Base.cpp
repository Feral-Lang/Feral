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
	static size_t tid = 0;
	return tid++;
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
