/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef LIBRARY_CORE_VEC_HPP
#define LIBRARY_CORE_VEC_HPP

#include "../../src/VM/VM.hpp"

var_base_t * vec_subs( vm_state_t & vm, const fn_data_t & fd )
{
	if( fd.args[ 1 ]->type() != VT_INT ) {
		vm.src_stack.back()->src()->fail( fd.idx, "expected integer argument for vector subscript, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	var_vec_t * data = VEC( fd.args[ 0 ] );
	mpz_class & sub = INT( fd.args[ 1 ] )->get();
	if( data->get().size() <= sub ) {
		vm.src_stack.back()->src()->fail( fd.idx, "subscript out of range, max capacity is: %zu, provided: %s",
						  data->get().size(), sub.get_str().c_str() );
		return nullptr;
	}
	return data->get()[ sub.get_ui() ];
}

#endif // LIBRARY_CORE_VEC_HPP