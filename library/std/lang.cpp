/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "../../src/VM/VM.hpp"

var_base_t * create_struct( vm_state_t & vm, const fn_data_t & fd )
{
	const size_t src_id = vm.src_stack.back()->src_id();
	std::vector< std::string > attr_order;
	std::unordered_map< std::string, var_base_t * > attrs;
	for( size_t i = 0; i < fd.assn_args.size(); ++i ) {
		auto & arg = fd.assn_args[ i ];
		attr_order.push_back( arg.name );
		attrs[ arg.name ] = arg.val->copy( src_id, fd.idx );
	}
	return make< var_struct_def_t >( attr_order, attrs );
}

REGISTER_MODULE( lang )
{
	var_src_t * src = vm.src_stack.back();
	const std::string & src_name = src->src()->path();
	src->add_nativefn( "struct", create_struct );
	return true;
}
