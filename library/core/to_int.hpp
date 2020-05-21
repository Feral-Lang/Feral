/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef LIBRARY_CORE_TO_INT_HPP
#define LIBRARY_CORE_TO_INT_HPP

#include "VM/VM.hpp"

var_base_t * nil_to_int( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( 0 );
}

var_base_t * bool_to_int( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( BOOL( fd.args[ 0 ] )->get() ? 1 : 0 );
}

var_base_t * typeid_to_int( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( TYPEID( fd.args[ 0 ] )->get() );
}

var_base_t * int_to_int( vm_state_t & vm, const fn_data_t & fd )
{
	return fd.args[ 0 ];
}

var_base_t * flt_to_int( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( FLT( fd.args[ 0 ] )->get().toInt() );
}

var_base_t * str_to_int( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( mpz_class( STR( fd.args[ 0 ] )->get() ) );
}

#endif // LIBRARY_CORE_TO_INT_HPP