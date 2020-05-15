/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
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
	std::unordered_map< std::string, var_base_t * > map_val;
	for( size_t i = 1; i < fd.args.size(); ++i ) {
		std::string key;
		if( !fd.args[ i ]->to_str( vm, key, fd.src_id, fd.idx ) ) {
			return nullptr;
		}
		if( map_val.find( key ) != map_val.end() ) var_dref( map_val[ key ] );
		map_val[ key ] = fd.args[ ++i ]->copy( fd.src_id, fd.idx );
	}
	return make< var_map_t >( map_val );
}

var_base_t * map_len( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( MAP( fd.args[ 0 ] )->get().size() );
}

var_base_t * map_empty( vm_state_t & vm, const fn_data_t & fd )
{
	return MAP( fd.args[ 0 ] )->get().empty() ? vm.tru : vm.fals;
}

var_base_t * map_insert( vm_state_t & vm, const fn_data_t & fd )
{
	std::unordered_map< std::string, var_base_t * > & map = MAP( fd.args[ 0 ] )->get();
	std::string key;
	if( !fd.args[ 1 ]->to_str( vm, key, fd.src_id, fd.idx ) ) {
		return nullptr;
	}
	if( map.find( key ) != map.end() ) {
		var_dref( map[ key ] );
	}
	var_iref( fd.args[ 2 ] );
	map[ key ] = fd.args[ 2 ];
	return fd.args[ 0 ];
}

var_base_t * map_erase( vm_state_t & vm, const fn_data_t & fd )
{
	std::unordered_map< std::string, var_base_t * > & map = MAP( fd.args[ 0 ] )->get();
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
	std::unordered_map< std::string, var_base_t * > & map = MAP( fd.args[ 0 ] )->get();
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
	std::unordered_map< std::string, var_base_t * > & map = MAP( fd.args[ 0 ] )->get();
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
	return res;
}

INIT_MODULE( map )
{
	var_src_t * src = vm.current_source();

	src->add_native_fn( "new", map_new, 0, true );

	vm.add_native_typefn< var_map_t >(    "len", map_len,    0, src_id, idx );
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
