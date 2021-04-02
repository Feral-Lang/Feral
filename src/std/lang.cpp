/*
	MIT License

	Copyright (c) 2020 Feral Language repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
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
	std::string res = vm.type_name( data->typefn_id() ) + "{";
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

var_base_t * struct_def_set_typename( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected argument to be of type string, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	vm.set_typename( STRUCT_DEF( fd.args[ 0 ] )->typefn_id(), STR( fd.args[ 1 ] )->get() );
	return fd.args[ 0 ];
}

var_base_t * struct_def_get_fields( vm_state_t & vm, const fn_data_t & fd )
{
	std::vector< var_base_t * > vec;
	const std::unordered_map< std::string, var_base_t * > & attrs = STRUCT_DEF( fd.args[ 0 ] )->attrs();
	for( auto & attr : attrs ) {
		vec.push_back( new var_str_t( attr.first, fd.src_id, fd.idx ) );
	}
	return make< var_vec_t >( vec, false );
}

var_base_t * struct_def_get_field_value( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected field name to be of type string, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	const std::string & attr = STR( fd.args[ 1 ] )->get();
	const std::unordered_map< std::string, var_base_t * > & attrs = STRUCT_DEF( fd.args[ 0 ] )->attrs();

	if( attrs.find( attr ) == attrs.end() ) return vm.nil;

	return attrs.at( attr );
}

var_base_t * struct_get_fields( vm_state_t & vm, const fn_data_t & fd )
{
	std::vector< var_base_t * > vec;
	const std::unordered_map< std::string, var_base_t * > & attrs = STRUCT( fd.args[ 0 ] )->attrs();
	for( auto & attr : attrs ) {
		vec.push_back( new var_str_t( attr.first, fd.src_id, fd.idx ) );
	}
	return make< var_vec_t >( vec, false );
}

var_base_t * struct_set_field_value( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected field name to be of type string, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	var_base_t * val = fd.args[ 2 ];
	const std::string & attr = STR( fd.args[ 1 ] )->get();
	const std::unordered_map< std::string, var_base_t * > & attrs = STRUCT( fd.args[ 0 ] )->attrs();

	const auto & res = attrs.find( attr );
	if( res == attrs.end() ) {
		vm.fail( fd.src_id, fd.idx, "field name '%s' not found",
			 attr.c_str() );
		return nullptr;
	}

	if( res->second->type() == val->type() ) {
		res->second->set( val );
	} else {
		vm.fail( fd.src_id, fd.idx, "attribute value type mismatch, provided '%s', existing '%s'",
			 vm.type_name( val ).c_str(), vm.type_name( res->second ).c_str() );
		return nullptr;
	}
	return vm.nil;
}

INIT_MODULE( lang )
{
	var_src_t * src = vm.current_source();
	src->add_native_fn( "enum", create_enum, 0, true );
	src->add_native_fn( "struct", create_struct );

	vm.add_native_typefn< var_struct_t >( "str", struct_to_str, 0, src_id, idx );

	vm.add_native_typefn< var_struct_def_t >( "set_typename", struct_def_set_typename, 1, src_id, idx );
	vm.add_native_typefn< var_struct_def_t >( "get_fields", struct_def_get_fields, 0, src_id, idx );
	vm.add_native_typefn< var_struct_def_t >( "[]", struct_def_get_field_value, 1, src_id, idx );

	vm.add_native_typefn< var_struct_t >( "get_fields", struct_get_fields, 0, src_id, idx );
	vm.add_native_typefn< var_struct_t >( "set_field", struct_set_field_value, 2, src_id, idx );

	return true;
}
