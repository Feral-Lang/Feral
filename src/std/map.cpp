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

#include "std/map_type.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////// Functions /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_base_t * map_new( vm_state_t & vm, const fn_data_t & fd )
{
	if( ( fd.args.size() - 1 ) % 2 != 0 ) {
		vm.fail( fd.src_id, fd.idx, "argument count must be even to create a map" );
		return nullptr;
	}
	bool refs = false;
	if( fd.assn_args_loc.find( "refs" ) != fd.assn_args_loc.end() ) {
		var_base_t * refs_var = fd.assn_args[ fd.assn_args_loc.at( "refs" ) ].val;
		if( !refs_var->istype< var_bool_t >() ) {
			vm.fail( fd.src_id, fd.idx, "expected 'refs' named argument to be of type bool for map.new(), found: %s",
				 vm.type_name( refs_var ).c_str() );
			return nullptr;
		}
		refs = BOOL( refs_var )->get();
	}
	std::map< std::string, var_base_t * > map_val;
	for( size_t i = 1; i < fd.args.size(); ++i ) {
		std::string key;
		if( !fd.args[ i ]->to_str( vm, key, fd.src_id, fd.idx ) ) {
			return nullptr;
		}
		if( map_val.find( key ) != map_val.end() ) var_dref( map_val[ key ] );
		if( refs ) {
			var_iref( fd.args[ ++i ] );
			map_val[ key ] = fd.args[ i ];
		} else {
			map_val[ key ] = fd.args[ ++i ]->copy( fd.src_id, fd.idx );
		}
	}
	return make< var_map_t >( map_val, refs );
}

var_base_t * map_len( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( MAP( fd.args[ 0 ] )->get().size() );
}

var_base_t * map_is_ref( vm_state_t & vm, const fn_data_t & fd )
{
	return MAP( fd.args[ 0 ] )->is_ref_map() ? vm.tru : vm.fals;
}

var_base_t * map_empty( vm_state_t & vm, const fn_data_t & fd )
{
	return MAP( fd.args[ 0 ] )->get().empty() ? vm.tru : vm.fals;
}

var_base_t * map_insert( vm_state_t & vm, const fn_data_t & fd )
{
	std::map< std::string, var_base_t * > & map = MAP( fd.args[ 0 ] )->get();
	std::string key;
	if( !fd.args[ 1 ]->to_str( vm, key, fd.src_id, fd.idx ) ) {
		return nullptr;
	}
	if( map.find( key ) != map.end() ) {
		var_dref( map[ key ] );
	}
	if( MAP( fd.args[ 0 ] )->is_ref_map() ) {
		var_iref( fd.args[ 2 ] );
		map[ key ] = fd.args[ 2 ];
	} else {
		map[ key ] = fd.args[ 2 ]->copy( fd.src_id, fd.idx );
	}
	return fd.args[ 0 ];
}

var_base_t * map_erase( vm_state_t & vm, const fn_data_t & fd )
{
	std::map< std::string, var_base_t * > & map = MAP( fd.args[ 0 ] )->get();
	std::string key;
	if( !fd.args[ 1 ]->to_str( vm, key, fd.src_id, fd.idx ) ) {
		return nullptr;
	}
	auto key_it = map.find( key );
	if( key_it != map.end() ) {
		var_dref( key_it->second );
		map.erase(key_it);
	}
	return fd.args[ 0 ];
}

var_base_t * map_get( vm_state_t & vm, const fn_data_t & fd )
{
	std::map< std::string, var_base_t * > & map = MAP( fd.args[ 0 ] )->get();
	std::string key;
	if( !fd.args[ 1 ]->to_str( vm, key, fd.src_id, fd.idx ) ) {
		return nullptr;
	}
	if( map.find( key ) == map.end() ) {
		return vm.nil;
	}
	return map[ key ];
}

var_base_t * map_find( vm_state_t & vm, const fn_data_t & fd )
{
	std::map< std::string, var_base_t * > & map = MAP( fd.args[ 0 ] )->get();
	std::string key;
	if( !fd.args[ 1 ]->to_str( vm, key, fd.src_id, fd.idx ) ) {
		return nullptr;
	}
	return map.find( key ) != map.end() ? vm.tru : vm.fals;
}

var_base_t * map_each( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_map_iterable_t >( MAP( fd.args[ 0 ] ) );
}

var_base_t * map_iterable_next( vm_state_t & vm, const fn_data_t & fd )
{
	var_map_iterable_t * it = MAP_ITERABLE( fd.args[ 0 ] );
	var_base_t * res = nullptr;
	if( !it->next( res, fd.src_id, fd.idx ) ) return vm.nil;
	res->set_load_as_ref();
	return res;
}

INIT_MODULE( map )
{
	var_src_t * src = vm.current_source();

	src->add_native_fn( "new", map_new, 0, true );

	vm.add_native_typefn< var_map_t >(    "len", map_len,    0, src_id, idx );
	vm.add_native_typefn< var_map_t >( "is_ref", map_is_ref, 0, src_id, idx );
	vm.add_native_typefn< var_map_t >(  "empty", map_empty,  0, src_id, idx );
	vm.add_native_typefn< var_map_t >( "insert", map_insert, 2, src_id, idx );
	vm.add_native_typefn< var_map_t >(  "erase", map_erase,  1, src_id, idx );
	vm.add_native_typefn< var_map_t >(    "get", map_get,    1, src_id, idx );
	vm.add_native_typefn< var_map_t >(     "[]", map_get,    1, src_id, idx );
	vm.add_native_typefn< var_map_t >(   "find", map_find,   1, src_id, idx );
	vm.add_native_typefn< var_map_t >(   "each", map_each,   0, src_id, idx );

	vm.add_native_typefn< var_map_iterable_t >( "next", map_iterable_next, 0, src_id, idx );

	return true;
}
