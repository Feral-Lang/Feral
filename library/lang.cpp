/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "../src/VM/VM.hpp"

var_base_t * create_struct( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.src_stack.back()->src();
	var_struct_t * st = make< var_struct_t >( fd.idx );
	for( size_t i = 0; i < fd.assn_args.size(); ++i ) {
		auto & arg = fd.assn_args[ i ];
		st->add_attr( arg.name, arg.val, true );
	}
	return st;
}

REGISTER_MODULE( lang )
{
	var_module_t * src = vm.src_stack.back();
	size_t src_id = vm.src_stack.size() - 1;
	src->vars()->add( "struct", new var_fn_t( src_id, "", ".", {}, {}, { .native = create_struct }, true, 0 ), vm.in_fn(), false );
	return true;
}
