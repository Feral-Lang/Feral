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

#include "VM/VM.hpp"

#include "std/bytebuffer_type.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////// Functions /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_base_t * bytebuffer_new_native( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected int argument for bytebuffer size, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	return make< var_bytebuffer_t >( mpz_get_ui( INT( fd.args[ 1 ] )->get() ) );
}

var_base_t * bytebuffer_resize( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected int argument for bytebuffer size, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	var_bytebuffer_t * self = BYTEBUFFER( fd.args[ 0 ] );
	self->resize( mpz_get_ui( INT( fd.args[ 1 ] )->get() ) );
	return fd.args[ 0 ];
}

var_base_t * bytebuffer_size( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( BYTEBUFFER( fd.args[ 0 ] )->get_size() );
}

var_base_t * bytebuffer_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	var_bytebuffer_t * self = BYTEBUFFER( fd.args[ 0 ] );
	if( self->get_size() == 0 ) return make< var_str_t >( "" );
	return make< var_str_t >( std::string( self->get_buf(), self->get_size() ) );
}

INIT_MODULE( bytebuffer )
{
	var_src_t * src = vm.current_source();
	src->add_native_fn( "new_native", bytebuffer_new_native, 1 );

	vm.add_native_typefn< var_bytebuffer_t >( "resize", bytebuffer_resize, 1, src_id, idx );
	vm.add_native_typefn< var_bytebuffer_t >( "len",    bytebuffer_size,   0, src_id, idx );
	vm.add_native_typefn< var_bytebuffer_t >( "str",    bytebuffer_to_str, 0, src_id, idx );
	return true;
}