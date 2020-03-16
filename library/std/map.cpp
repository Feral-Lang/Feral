/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "../../src/VM/VM.hpp"

var_base_t * map_new( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.src_stack.back()->src();
	if( ( fd.args.size() - 1 ) % 2 != 0 ) {
		src->fail( fd.idx, "argument count must be even to create a map" );
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

var_base_t * map_insert( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.src_stack.back()->src();
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
	srcfile_t * src = vm.src_stack.back()->src();
	std::unordered_map< std::string, var_base_t * > & map = MAP( fd.args[ 0 ] )->get();
	std::string key;
	if( !fd.args[ 1 ]->to_str( vm, key, fd.src_id, fd.idx ) ) {
		return nullptr;
	}
	if( map.find( key ) != map.end() ) {
		var_dref( map[ key ] );
	}
	return fd.args[ 0 ];
}

var_base_t * map_get( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.src_stack.back()->src();
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
	srcfile_t * src = vm.src_stack.back()->src();
	std::unordered_map< std::string, var_base_t * > & map = MAP( fd.args[ 0 ] )->get();
	std::string key;
	if( !fd.args[ 1 ]->to_str( vm, key, fd.src_id, fd.idx ) ) {
		return nullptr;
	}
	return map.find( key ) != map.end() ? vm.tru : vm.fals;
}

INIT_MODULE( map )
{
	var_src_t * src = vm.src_stack.back();
	const std::string & src_name = src->src()->path();

	src->add_nativefn( "new", map_new, {}, {}, true );

	vm.add_typefn( VT_MAP, "insert", new var_fn_t( src_name, "",  "", { "", "" }, {}, { .native = map_insert }, true, 0, 0 ), false );
	vm.add_typefn( VT_MAP, "get", new var_fn_t( src_name, "",  "", { "" }, {}, { .native = map_get }, true, 0, 0 ), false );
	vm.add_typefn( VT_MAP,  "[]", new var_fn_t( src_name, "",  "", { "" }, {}, { .native = map_get }, true, 0, 0 ), false );
	vm.add_typefn( VT_MAP,  "find", new var_fn_t( src_name, "",  "", { "" }, {}, { .native = map_find }, true, 0, 0 ), false );

	return true;
}
