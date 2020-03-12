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

REGISTER_MODULE( sys )
{
	var_src_t * src = vm.src_stack.back();
	const std::string & src_name = src->src()->path();
	src->add_nativefn( "exit", _exit, {}, true );
	return true;
}
