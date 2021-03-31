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

#ifndef LIBRARY_CORE_TO_FLT_HPP
#define LIBRARY_CORE_TO_FLT_HPP

#include "VM/VM.hpp"

var_base_t * nil_to_flt( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_flt_t >( 0.0 );
}

var_base_t * bool_to_flt( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_flt_t >( BOOL( fd.args[ 0 ] )->get() ? 1.0 : 0.0 );
}

var_base_t * int_to_flt( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_flt_t >( INT( fd.args[ 0 ] )->get() );
}

var_base_t * flt_to_flt( vm_state_t & vm, const fn_data_t & fd )
{
	return fd.args[ 0 ];
}

var_base_t * str_to_flt( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "base must be an integer, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	var_flt_t * res = make< var_flt_t >( 0.0 );
	int tmp = mpfr_set_str( res->get(), STR( fd.args[ 0 ] )->get().c_str(), mpz_get_ui( INT( fd.args[ 1 ] )->get() ), mpfr_get_default_rounding_mode() );
	if( tmp == 0 ) return res;
	return vm.nil;
}

#endif // LIBRARY_CORE_TO_FLT_HPP