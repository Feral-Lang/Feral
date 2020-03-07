/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "../src/VM/VM.hpp"

// var_base_t * create_map( vm_state_t & vm, const fn_data_t & fd )
// {
// 	srcfile_t * src = vm.src_stack.back()->src();
// 	if( ( fd.args.size() - 1 ) % 2 != 0 ) {
// 		src->fail( fd.idx, "argument count must be even to create a map" );
// 		return nullptr;
// 	}
// 	std::unordered_map< std::string, var_base_t * > map_val;
// 	for( size_t i = 1; i < fd.args.size(); ++i ) {
// 		auto & key = fd.args[ i ];
// 		var_base_t * str_fn = key->get_attr( "str" );
// 		if( str_fn == nullptr || str_fn->type() != VT_FUNC ) {
// 			src->fail( key->idx(), "type of this variable does not implement a 'str' function to allow its usage as key" );
// 			return nullptr;
// 		}
// 		if( !FN( str_fn )->call( vm, { key }, {}, fd.idx ) ) {
// 			src->fail( key->idx(), "failed to call 'str' function (make sure the argument count is correct)" );
// 			return nullptr;
// 		}
// 		var_base_t * str = vm.vm_stack->back();
// 		if( str->type() != VT_STR ) {
// 			src->fail( key->idx(), "expected string return type from 'str' function, received: %zu", str->type() );
// 			vm.vm_stack->pop_back();
// 			return nullptr;
// 		}
// 		if( map_val.find( STR( str )->get() ) != map_val.end() ) var_dref( map_val[ STR( str )->get() ] );
// 		map_val[ STR( str )->get() ] = fd.args[ ++i ]->base_copy( fd.idx );
// 		vm.vm_stack->pop_back();
// 	}
// 	return make< var_map_t >( map_val, fd.idx );
// }

REGISTER_MODULE( map )
{
	var_src_t * src = vm.src_stack.back();
	// const std::string & src_name = src->src()->get_path();
	// src->vars()->add( "new", new var_fn_t( src_name, "", ".", {}, {}, { .native = create_map }, true, 0 ), vm.in_fn(), false );
	return true;
}
