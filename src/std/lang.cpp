/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "VM/VM.hpp"

#include "std/struct_type.hpp"

static uintptr_t gen_struct_enum_id()
{
	static uintptr_t id = 0;
	return id++;
}

var_base_t * create_struct( vm_state_t & vm, const fn_data_t & fd )
{
	const size_t src_id = vm.current_source()->src_id();
	std::vector< std::string > attr_order;
	std::unordered_map< std::string, var_base_t * > attrs;
	for( size_t i = 0; i < fd.assn_args.size(); ++i ) {
		auto & arg = fd.assn_args[ i ];
		attr_order.push_back( arg.name );
		attrs[ arg.name ] = arg.val->copy( src_id, fd.idx );
	}
	return make< var_struct_def_t >( gen_struct_enum_id(), attr_order, attrs );
}

var_base_t * create_enum( vm_state_t & vm, const fn_data_t & fd )
{
	std::unordered_map< std::string, var_base_t * > attrs;

	for( size_t i = 1; i < fd.args.size(); ++i ) {
		auto & arg = fd.args[ i ];
		if( !arg->istype< var_str_t >() ) {
			vm.fail( arg->src_id(), arg->idx(), "expected const strings for enums (use strings or atoms)" );
			goto fail;
		}
		attrs[ STR( arg )->get() ] = new var_int_t( i - 1, fd.src_id, fd.idx );
	}

	for( auto & arg : fd.assn_args ) {
		if( !arg.val->istype< var_int_t >() ) {
			vm.fail( arg.src_id, arg.idx, "expected argument value to be of integer for enums, found: %s",
				 vm.type_name( arg.val ).c_str() );
			goto fail;
		}
		if( attrs.find( arg.name ) != attrs.end() ) {
			var_dref( attrs[ arg.name ] );
		}
		attrs[ arg.name ] = arg.val->copy( fd.src_id, fd.idx );
	}

	return make< var_struct_t >( gen_struct_enum_id(), attrs, nullptr );
fail:
	for( auto & attr : attrs ) {
		var_dref( attr.second );
	}
	return nullptr;
}

var_base_t * struct_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	var_struct_t * data = STRUCT( fd.args[ 0 ] );
	std::string res = vm.type_name( data ) + "{";
	for( auto & e : data->attrs() ) {
		std::string str;
		if( !e.second->to_str( vm, str, fd.src_id, fd.idx ) ) {
			return nullptr;
		}
		res += e.first + ": " + str + ", ";
	}
	if( data->attrs().size() > 0 ) {
		res.pop_back();
		res.pop_back();
	}
	res += "}";
	return make< var_str_t >( res );
}

INIT_MODULE( lang )
{
	var_src_t * src = vm.current_source();
	src->add_native_fn( "enum", create_enum, 0, true );
	src->add_native_fn( "struct", create_struct );

	vm.add_native_typefn< var_struct_t >( "str", struct_to_str, 0, src_id, idx );
	return true;
}
