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

#include "Compiler/Config.hpp"

#include "VM/VM.hpp"

var_base_t * _exit( vm_state_t & vm, const fn_data_t & fd )
{
	vm.exit_called = true;
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected integer for exit function parameter - exit code" );
		return nullptr;
	}
	vm.exit_code = mpz_get_si( INT( fd.args[ 1 ] )->get() );
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

var_base_t * set_call_stack_max( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected int argument for max count, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	vm.exec_stack_max = mpz_get_ui( INT( fd.args[ 1 ] )->get() );
	return vm.nil;
}

var_base_t * get_call_stack_max( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( vm.exec_stack_max );
}

INIT_MODULE( sys )
{
	var_src_t * src = vm.current_source();

	src->add_native_fn( "exit_native", _exit, 1 );
	src->add_native_fn( "var_exists", var_exists, 1 );
	src->add_native_fn( "set_call_stack_max_native", set_call_stack_max, 1 );
	src->add_native_fn( "get_call_stack_max", get_call_stack_max, 0 );

	src->add_native_var( "args", vm.src_args );

	src->add_native_var( "install_prefix", make_all< var_str_t >( STRINGIFY( INSTALL_PREFIX ), src_id, idx ) );

	src->add_native_var( "self_bin", make_all< var_str_t >( vm.self_bin(), src_id, idx ) );
	src->add_native_var( "self_base", make_all< var_str_t >( vm.self_base(), src_id, idx ) );

	src->add_native_var( "version_major", make_all< var_int_t >( FERAL_VERSION_MAJOR, src_id, idx ) );
	src->add_native_var( "version_minor", make_all< var_int_t >( FERAL_VERSION_MINOR, src_id, idx ) );
	src->add_native_var( "version_patch", make_all< var_int_t >( FERAL_VERSION_PATCH, src_id, idx ) );

	src->add_native_var( "build_date", make_all< var_str_t >( BUILD_DATE, src_id, idx ) );
	src->add_native_var( "build_compiler", make_all< var_str_t >( BUILD_CXX_COMPILER, src_id, idx ) );

	src->add_native_var( "CALL_STACK_MAX_DEFAULT", make_all< var_int_t >( EXEC_STACK_MAX_DEFAULT, src_id, idx ) );
	return true;
}
