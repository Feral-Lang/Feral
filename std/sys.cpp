/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "../src/VM/VM.hpp"

var_base_t * _exit( vm_state_t & vm, const fn_data_t & fd )
{
	vm.exit_called = true;
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected integer for exit function parameter - exit code" );
		return nullptr;
	}
	vm.exit_code = INT( fd.args[ 1 ] )->get().get_si();
	return vm.nil;
}

var_base_t * var_exists( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected string argument for variable name, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	return vm.current_source()->vars()->get( STR( fd.args[ 1 ] )->get() ) != nullptr ? vm.tru : vm.fals;
}

INIT_MODULE( sys )
{
	var_src_t * src = vm.current_source();

	src->add_native_fn( "exit_native", _exit, 1 );
	src->add_native_fn( "var_exists", var_exists, 1 );

	src->add_native_var( "args", vm.src_args );

	src->add_native_var( "self_binary", make_all< var_str_t >( vm.self_binary(), src_id, idx ) );
	src->add_native_var( "prefix",  make_all< var_str_t >(  vm.prefix(), src_id, idx ) );
	return true;
}
