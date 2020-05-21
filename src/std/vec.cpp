/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "VM/VM.hpp"

#include "std/vec_type.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////// Functions /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_base_t * vec_new( vm_state_t & vm, const fn_data_t & fd )
{
	std::vector< var_base_t * > vec_val;
	size_t reserve_size = fd.args.size() - 1;
	bool refs = false;
	if( fd.assn_args_loc.find( "refs" ) != fd.assn_args_loc.end() ) {
		var_base_t * refs_var = fd.assn_args[ fd.assn_args_loc.at( "refs" ) ].val;
		if( !refs_var->istype< var_bool_t >() ) {
			vm.fail( fd.src_id, fd.idx, "expected 'refs' named argument to be of type bool for vec.new(), found: %s",
				 vm.type_name( refs_var ).c_str() );
			return nullptr;
		}
		refs = BOOL( refs_var )->get();
	}
	if( fd.assn_args_loc.find( "size" ) != fd.assn_args_loc.end() ) {
		var_base_t * size_var = fd.assn_args[ fd.assn_args_loc.at( "size" ) ].val;
		if( !size_var->istype< var_int_t >() ) {
			vm.fail( fd.src_id, fd.idx, "expected 'size' named argument to be of type int for vec.new(), found: %s",
				 vm.type_name( size_var ).c_str() );
			return nullptr;
		}
		reserve_size = INT( size_var )->get().get_ui();
	}
	vec_val.reserve( reserve_size );
	if( refs ) {
		for( size_t i = 1; i < fd.args.size(); ++i ) {
			var_iref( fd.args[ i ] );
			vec_val.push_back( fd.args[ i ] );
		}
	} else {
		for( size_t i = 1; i < fd.args.size(); ++i ) {
			vec_val.push_back( fd.args[ i ]->copy( fd.src_id, fd.idx ) );
		}
	}
	return make< var_vec_t >( vec_val, refs );
}

var_base_t * vec_size( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( VEC( fd.args[ 0 ] )->get().size() );
}

var_base_t * vec_empty( vm_state_t & vm, const fn_data_t & fd )
{
	return VEC( fd.args[ 0 ] )->get().size() == 0 ? vm.tru : vm.fals;
}

var_base_t * vec_each( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_vec_iterable_t >( VEC( fd.args[ 0 ] ) );
}

var_base_t * vec_front( vm_state_t & vm, const fn_data_t & fd )
{
	std::vector< var_base_t * > & vec = VEC( fd.args[ 0 ] )->get();
	return vec.size() == 0 ? vm.nil : vec.front();
}

var_base_t * vec_back( vm_state_t & vm, const fn_data_t & fd )
{
	std::vector< var_base_t * > & vec = VEC( fd.args[ 0 ] )->get();
	return vec.size() == 0 ? vm.nil : vec.back();
}

var_base_t * vec_push( vm_state_t & vm, const fn_data_t & fd )
{
	std::vector< var_base_t * > & vec = VEC( fd.args[ 0 ] )->get();
	if( VEC( fd.args[ 0 ] )->is_ref_vec() ) {
		var_iref( fd.args[ 1 ] );
		vec.push_back( fd.args[ 1 ] );
	} else {
		vec.push_back( fd.args[ 1 ]->copy( fd.src_id, fd.idx ) );
	}
	return fd.args[ 0 ];
}

var_base_t * vec_pop( vm_state_t & vm, const fn_data_t & fd )
{
	std::vector< var_base_t * > & vec = VEC( fd.args[ 0 ] )->get();
	if( vec.empty() ) {
		vm.fail( fd.src_id, fd.idx, "performed pop() on an empty vector" );
		return nullptr;
	}
	var_dref( vec.back() );
	vec.pop_back();
	return fd.args[ 0 ];
}

var_base_t * vec_setat( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected first argument to be of type integer for vec.set(), found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	size_t pos = INT( fd.args[ 1 ] )->get().get_ui();
	std::vector< var_base_t * > & vec = VEC( fd.args[ 0 ] )->get();
	if( pos >= vec.size() ) {
		vm.fail( fd.src_id, fd.idx, "position %zu is not within string of length %zu",
			 pos, vec.size() );
		return nullptr;
	}
	var_dref( vec[ pos ] );
	if( VEC( fd.args[ 0 ] )->is_ref_vec() ) {
		var_iref( fd.args[ 2 ] );
		vec[ pos ] = fd.args[ 2 ];
	} else {
		vec[ pos ] = fd.args[ 2 ]->copy( fd.src_id, fd.idx );
	}
	return fd.args[ 0 ];
}

var_base_t * vec_insert( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected first argument to be of type integer for string.insert(), found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	size_t pos = INT( fd.args[ 1 ] )->get().get_ui();
	std::vector< var_base_t * > & vec = VEC( fd.args[ 0 ] )->get();
	if( pos > vec.size() ) {
		vm.fail( fd.src_id, fd.idx, "position %zu is greater than vector length %zu",
			 pos, vec.size() );
		return nullptr;
	}
	if( VEC( fd.args[ 0 ] )->is_ref_vec() ) {
		var_iref( fd.args[ 2 ] );
		vec.insert( vec.begin() + pos, fd.args[ 2 ] );
	} else {
		vec.insert( vec.begin() + pos, fd.args[ 2 ]->copy( fd.src_id, fd.idx ) );
	}
	return fd.args[ 0 ];
}

var_base_t * vec_erase( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected argument to be of type integer for string.erase(), found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	size_t pos = INT( fd.args[ 1 ] )->get().get_ui();
	std::vector< var_base_t * > & vec = VEC( fd.args[ 0 ] )->get();
	if( pos >= vec.size() ) {
		vm.fail( fd.src_id, fd.idx, "attempted erase on pos: %zu, vector size: %zu",
			 pos, vec.size() );
		return nullptr;
	}
	var_dref( vec[ pos ] );
	vec.erase( vec.begin() + pos );
	return fd.args[ 0 ];
}

var_base_t * vec_last( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( VEC( fd.args[ 0 ] )->get().size() - 1 );
}

var_base_t * vec_at( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected argument to be of type integer for string.erase(), found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	std::vector< var_base_t * > & vec = VEC( fd.args[ 0 ] )->get();
	size_t pos = INT( fd.args[ 1 ] )->get().get_ui();
	if( pos >= vec.size() ) return vm.nil;
	return vec[ pos ];
}

var_base_t * vec_iterable_next( vm_state_t & vm, const fn_data_t & fd )
{
	var_vec_iterable_t * it = VEC_ITERABLE( fd.args[ 0 ] );
	var_base_t * res = nullptr;
	if( !it->next( res ) ) return vm.nil;
	return res;
}

var_base_t * vec_slice( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected starting index to be of type 'int' for vec.slice(), found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( !fd.args[ 2 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected ending index to be of type 'int' for vec.slice(), found: %s",
			 vm.type_name( fd.args[ 2 ] ).c_str() );
		return nullptr;
	}

	std::vector< var_base_t * > & vec = VEC( fd.args[ 0 ] )->get();
	size_t begin = INT( fd.args[ 1 ] )->get().get_ui();
	size_t end = INT( fd.args[ 2 ] )->get().get_ui();

	std::vector< var_base_t * > newvec;
	if( end > begin ) newvec.reserve( end - begin );
	for( size_t i = begin; i < end; ++i ) {
		var_iref( vec[ i ] );
		newvec.push_back( vec[ i ] );
	}
	return make< var_vec_t >( newvec, true );
}

INIT_MODULE( vec )
{
	var_src_t * src = vm.current_source();

	src->add_native_fn( "new", vec_new, 0, true );

	vm.add_native_typefn< var_vec_t >(     "len",    vec_size, 0, src_id, idx );
	vm.add_native_typefn< var_vec_t >(   "empty",   vec_empty, 0, src_id, idx );
	vm.add_native_typefn< var_vec_t >(   "front",   vec_front, 0, src_id, idx );
	vm.add_native_typefn< var_vec_t >(    "back",    vec_back, 0, src_id, idx );
	vm.add_native_typefn< var_vec_t >(    "push",    vec_push, 1, src_id, idx );
	vm.add_native_typefn< var_vec_t >(     "pop",     vec_pop, 0, src_id, idx );
	vm.add_native_typefn< var_vec_t >(  "insert",  vec_insert, 2, src_id, idx );
	vm.add_native_typefn< var_vec_t >(   "erase",   vec_erase, 1, src_id, idx );
	vm.add_native_typefn< var_vec_t >( "lastidx",    vec_last, 0, src_id, idx );
	vm.add_native_typefn< var_vec_t >(     "set",   vec_setat, 2, src_id, idx );
	vm.add_native_typefn< var_vec_t >(      "at",      vec_at, 1, src_id, idx );
	vm.add_native_typefn< var_vec_t >(      "[]",      vec_at, 1, src_id, idx );
	vm.add_native_typefn< var_vec_t >(    "each",    vec_each, 0, src_id, idx );

	vm.add_native_typefn< var_vec_t >( "slice_native", vec_slice, 2, src_id, idx );

	vm.add_native_typefn< var_vec_iterable_t >( "next", vec_iterable_next, 0, src_id, idx );

	return true;
}
