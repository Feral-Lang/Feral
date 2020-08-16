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

#include <chrono>

#include "VM/VM.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////// Functions /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_base_t * stdclk_now( vm_state_t & vm, const fn_data_t & fd )
{
	var_int_t * res = make< var_int_t >( 0 );
	mpz_set_ui( res->get(), std::chrono::duration_cast< std::chrono::nanoseconds >( std::chrono::steady_clock::now().time_since_epoch() ).count() );
	return res;
}

var_base_t * sysclk_now( vm_state_t & vm, const fn_data_t & fd )
{
	var_int_t * res = make< var_int_t >( 0 );
	mpz_set_ui( res->get(), std::chrono::duration_cast< std::chrono::nanoseconds >( std::chrono::system_clock::now().time_since_epoch() ).count() );
	return res;
}

INIT_MODULE( time )
{
	var_src_t * src = vm.current_source();
	src->add_native_fn( "steady_clock_now_native", stdclk_now );
	src->add_native_fn( "system_clock_now_native", sysclk_now );
	return true;
}