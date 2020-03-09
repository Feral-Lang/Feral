/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "../../src/VM/VM.hpp"

// var_base_t * create_vector( vm_state_t & vm, const fn_data_t & fd )
// {
// 	std::vector< var_base_t * > vec_val;
// 	for( size_t i = 1; i < fd.args.size(); ++i ) {
// 		vec_val.push_back( fd.args[ i ]->base_copy( fd.idx ) );
// 	}
// 	return make< var_vec_t >( vec_val );
// }

var_base_t * vec_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.src_stack.back()->src();
	var_vec_t * vec = VEC( fd.args[ 0 ] );
	std::string res = "[";
	for( auto & e : vec->get() ) {
		var_fn_t * str_fn = vm.get_typefn( e->type(), "str" );
		if( !str_fn ) str_fn = vm.get_typefn( VT_ALL, "str" );
		if( !str_fn ) {
			src->fail( e->idx(), "no 'str' function implement for type: '%zu' or global type", e->type() );
			return nullptr;
		}
		if( !FN( str_fn )->call( vm, { e }, {}, src->id(), fd.idx ) ) {
			src->fail( e->idx(), "function call 'str' for type: %zu failed", e->type() );
			return nullptr;
		}
		var_base_t * str = vm.vm_stack->pop( false );
		if( str->type() != VT_STR ) {
			src->fail( e->idx(), "expected string return type from 'str' function, received: %zu", str->type() );
			var_dref( str );
			return nullptr;
		}
		res += STR( str )->get() + ", ";
		var_dref( str );
	}
	if( vec->get().size() > 0 ) {
		res.pop_back();
		res.pop_back();
	}
	res += "]";
	return make< var_str_t >( res );
}

REGISTER_MODULE( vec )
{
	var_src_t * src = vm.src_stack.back();
	const std::string & src_name = src->src()->path();
	// src->vars()->add( "new", new var_fn_t( src_name, "", ".", {}, {}, { .native = create_vector }, true, 0 ), vm.in_fn(), false );
	vm.add_typefn( VT_VEC, "str", new var_fn_t( src_name, {}, { .native = vec_to_str },  0, 0 ), false );
	return true;
}
