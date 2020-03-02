/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "../src/VM/VM.hpp"

var_base_t * println( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.src_stack.back()->src();
	for( size_t i = 1; i < fd.args.size(); ++i ) {
		auto & arg = fd.args[ i ];
		var_base_t * str = arg->call_fn_result( vm, "str", {}, fd.idx );
		if( !str ) return nullptr;
		if( str->type() != VT_STR ) {
			src->fail( arg->idx(), "expected string return type from 'str' function, received: %zu", str->type() );
			vm.vm_stack->pop_back();
			return nullptr;
		}
		fprintf( stdout, "%s", STR( str )->get().c_str() );
		if( i < fd.args.size() - 1 ) fprintf( stdout, " " );
		vm.vm_stack->pop_back();
	}
	fprintf( stdout, "\n" );
	return vm.nil;
}

REGISTER_MODULE( io )
{
	var_module_t * src = vm.src_stack.back();
	const std::string & src_name = src->src()->get_path();
	src->vars()->add( "println", new var_fn_t( src_name, "", ".", {}, {}, { .native = println }, true, 0 ), vm.in_fn(), false );
	return true;
}
