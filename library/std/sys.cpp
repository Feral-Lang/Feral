/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "../../src/VM/VM.hpp"

var_base_t * _exit( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src_file = vm.src_stack.back()->src();
	vm.exit_called = true;
	if( fd.args.size() <= 1 ) {
		vm.exit_code = 0;
		return vm.nil;
	}
	if( fd.args[ 1 ]->type() != VT_INT ) {
		src_file->fail( fd.idx, "expected integer for exit function parameter - exit code" );
		return nullptr;
	}
	vm.exit_code = INT( fd.args[ 1 ] )->get().get_si();
	return vm.nil;
}

var_base_t * self_binary_loc( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_str_t >( vm.self_binary() );
}

var_base_t * src_args( vm_state_t & vm, const fn_data_t & fd )
{
	return vm.src_args;
}

var_base_t * inc_load_loc( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_str_t >( vm.inc_locs().front() );
}

var_base_t * dll_load_loc( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_str_t >( vm.dll_locs().front() );
}

var_base_t * dll_core_load_loc( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_str_t >( vm.dll_core_load_loc() );
}

var_base_t * feral_home_dir( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_str_t >( vm.feral_home_dir() );
}

INIT_MODULE( sys )
{
	var_src_t * src = vm.src_stack.back();
	const std::string & src_name = src->src()->path();
	src->add_nativefn( "exit", _exit, {}, {}, true );
	src->add_nativefn( "self_binary_loc_native", self_binary_loc );
	src->add_nativefn( "src_args_native", src_args );
	src->add_nativefn( "inc_load_loc_native", inc_load_loc );
	src->add_nativefn( "dll_load_loc_native", dll_load_loc );
	src->add_nativefn( "dll_core_load_loc_native", dll_core_load_loc );
	src->add_nativefn( "feral_home_dir_native", feral_home_dir );
	return true;
}
