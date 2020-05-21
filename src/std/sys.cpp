/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Compiler/Config.hpp"

#include "VM/VM.hpp"

#define _STRINGIZE(x) #x
#define STRINGIFY(x) _STRINGIZE(x)

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

	src->add_native_var( "install_prefix", make_all< var_str_t >( STRINGIFY( INSTALL_PREFIX ), src_id, idx ) );

	src->add_native_var( "self_bin", make_all< var_str_t >( vm.self_bin(), src_id, idx ) );
	src->add_native_var( "self_base", make_all< var_str_t >( vm.self_base(), src_id, idx ) );

	src->add_native_var( "version_major", make_all< var_int_t >( FERAL_VERSION_MAJOR, src_id, idx ) );
	src->add_native_var( "version_minor", make_all< var_int_t >( FERAL_VERSION_MINOR, src_id, idx ) );
	src->add_native_var( "version_patch", make_all< var_int_t >( FERAL_VERSION_PATCH, src_id, idx ) );

	src->add_native_var( "build_date", make_all< var_str_t >( BUILD_DATE, src_id, idx ) );
	src->add_native_var( "build_compiler", make_all< var_str_t >( BUILD_CXX_COMPILER, src_id, idx ) );
	return true;
}
