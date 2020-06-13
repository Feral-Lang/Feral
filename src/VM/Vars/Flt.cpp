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

#include "VM/Vars/Base.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// VAR_INT //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_flt_t::var_flt_t( const mpfr_t val, const size_t & src_id, const size_t & idx )
	: var_base_t( type_id< var_flt_t >(), src_id, idx, false, false )
{
	mpfr_init_set( m_val, val, mpfr_get_default_rounding_mode() );
}
var_flt_t::var_flt_t( const double & val, const size_t & src_id, const size_t & idx )
	: var_base_t( type_id< var_flt_t >(), src_id, idx, false, false )
{
	mpfr_init_set_d( m_val, val, mpfr_get_default_rounding_mode() );
}
var_flt_t::var_flt_t( const mpz_t val, const size_t & src_id, const size_t & idx )
	: var_base_t( type_id< var_flt_t >(), src_id, idx, false, false )
{
	mpfr_init_set_z( m_val, val, mpfr_get_default_rounding_mode() );
}
var_flt_t::var_flt_t( const char * val, const size_t & src_id, const size_t & idx )
	: var_base_t( type_id< var_flt_t >(), src_id, idx, false, false )
{
	mpfr_init_set_str( m_val, val, 0, mpfr_get_default_rounding_mode() );
}
var_flt_t::~var_flt_t()
{
	mpfr_clear( m_val );
}

var_base_t * var_flt_t::copy( const size_t & src_id, const size_t & idx ) { return new var_flt_t( m_val, src_id, idx ); }
mpfr_t & var_flt_t::get() { return m_val; }
void var_flt_t::set( var_base_t * from )
{
	mpfr_set( m_val, FLT( from )->get(), mpfr_get_default_rounding_mode() );
}