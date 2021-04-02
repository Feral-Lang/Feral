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

var_base_t * sysclk_now( vm_state_t & vm, const fn_data_t & fd )
{
	var_int_t * res = make< var_int_t >( 0 );
	mpz_set_ui( res->get(), std::chrono::duration_cast< std::chrono::nanoseconds >( std::chrono::system_clock::now().time_since_epoch() ).count() );
	return res;
}

var_base_t * time_format( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected integer argument as time for formatting, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( !fd.args[ 2 ]->istype< var_str_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected string argument as format for time formatting, found: %s",
			 vm.type_name( fd.args[ 2 ] ).c_str() );
		return nullptr;
	}
	unsigned long val = mpz_get_ui( INT( fd.args[ 1 ] )->get() );
	std::chrono::nanoseconds nsval( val );
	std::chrono::system_clock::time_point tp( ( std::chrono::nanoseconds( val ) ) );
	std::time_t time = std::chrono::system_clock::to_time_t( tp );
	std::tm * t = std::localtime( & time );
	char fmt[ 1024 ] = { 0 };
	if( std::strftime( fmt, sizeof( fmt ), STR( fd.args[ 2 ] )->get().c_str(), t ) ) {
		return make< var_str_t >( fmt );
	}
	return vm.nil;
}

INIT_MODULE( time )
{
	var_src_t * src = vm.current_source();
	src->add_native_fn( "system_clock_now_native", sysclk_now );
	src->add_native_fn( "format_native", time_format, 2 );
	return true;
}