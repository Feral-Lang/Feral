/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "VM/VM.hpp"

#include "std/ptr_type.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////// Functions /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_base_t * ptr_new_native( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_ptr_t >( fd.args[ 1 ] );
}

var_base_t * ptr_set( vm_state_t & vm, const fn_data_t & fd )
{
	var_ptr_t * self = PTR( fd.args[ 0 ] );
	self->update( fd.args[ 1 ] );
	return fd.args[ 0 ];
}

var_base_t * ptr_get( vm_state_t & vm, const fn_data_t & fd )
{
	return PTR( fd.args[ 0 ] )->get();
}

INIT_MODULE( ptr )
{
	var_src_t * src = vm.current_source();
	src->add_native_fn( "new_native", ptr_new_native, 1 );

	vm.add_native_typefn< var_ptr_t >( "set", ptr_set, 1, src_id, idx );
	vm.add_native_typefn< var_ptr_t >( "get", ptr_get, 0, src_id, idx );
	return true;
}