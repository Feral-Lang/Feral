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

REGISTER_MODULE( vec )
{
	var_src_t * src = vm.src_stack.back();
	// const std::string & src_name = src->src()->get_path();
	// src->vars()->add( "new", new var_fn_t( src_name, "", ".", {}, {}, { .native = create_vector }, true, 0 ), vm.in_fn(), false );
	return true;
}
