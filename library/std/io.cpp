/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "../../src/VM/VM.hpp"

var_base_t * println( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.src_stack.back()->src();
	for( size_t i = 1; i < fd.args.size(); ++i ) {
		auto & arg = fd.args[ i ];
		var_fn_t * str_fn = vm.get_typefn( arg->type(), "str" );
		if( !str_fn ) str_fn = vm.get_typefn( VT_ALL, "str" );
		if( !str_fn ) {
			src->fail( arg->idx(), "no 'str' function implement for type: '%zu' or global type", arg->type() );
			return nullptr;
		}
		if( !FN( str_fn )->call( vm, { arg }, {}, src->id(), fd.idx ) ) {
			src->fail( arg->idx(), "function call 'str' for type: %zu failed", arg->type() );
			return nullptr;
		}
		var_base_t * str = vm.vm_stack->pop( false );
		if( str->type() != VT_STR ) {
			src->fail( arg->idx(), "expected string return type from 'str' function, received: %zu", str->type() );
			var_dref( str );
			return nullptr;
		}
		fprintf( stdout, "%s", STR( str )->get().c_str() );
		if( i < fd.args.size() - 1 ) fprintf( stdout, " " );
		var_dref( str );
	}
	fprintf( stdout, "\n" );
	return vm.nil;
}

REGISTER_MODULE( io )
{
	var_src_t * src = vm.src_stack.back();
	const std::string & src_name = src->src()->path();
	src->vars()->add( "println", new var_fn_t( src_name, "", ".", {}, {}, { .native = println }, true, 0, 0 ), false );
	return true;
}
