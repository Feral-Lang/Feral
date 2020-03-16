/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "../../src/VM/VM.hpp"

static int register_struct_enum_id()
{
	static int id = _VT_LAST;
	return id++;
}

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
	return make< var_struct_def_t >( register_struct_enum_id(), attr_order, attrs );
}

var_base_t * create_enum( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src_file = vm.src_stack.back()->src();
	std::unordered_map< std::string, var_base_t * > attrs;

	for( size_t i = 1; i < fd.args.size(); ++i ) {
		auto & arg = fd.args[ i ];
		if( arg->type() != VT_STR ) {
			src_file->fail( arg->idx(), "expected const strings for enums (use strings or atoms)" );
			goto fail;
		}
		attrs[ STR( arg )->get() ] = new var_int_t( i - 1, fd.src_id, fd.idx );
	}

	for( auto & arg : fd.assn_args ) {
		if( arg.val->type() != VT_INT ) {
			src_file->fail( arg.idx, "expected argument value to be of integer for enums, found: %s",
					vm.type_name( arg.val->type() ).c_str() );
			goto fail;
		}
		if( attrs.find( arg.name ) != attrs.end() ) {
			var_dref( attrs[ arg.name ] );
		}
		attrs[ arg.name ] = arg.val->copy( fd.src_id, fd.idx );
	}

	return make< var_struct_t >( register_struct_enum_id(), attrs );
fail:
	for( auto & attr : attrs ) {
		var_dref( attr.second );
	}
	return nullptr;
}

REGISTER_MODULE( lang )
{
	var_src_t * src = vm.src_stack.back();
	const std::string & src_name = src->src()->path();
	src->add_nativefn( "enum", create_enum, {}, {}, true );
	src->add_nativefn( "struct", create_struct );
	return true;
}
