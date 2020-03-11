/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "../../src/VM/VM.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////// Functions /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector< var_base_t * > _str_split( const std::string & data, const char delim,
					const size_t & src_id, const size_t & idx );

var_base_t * str_size( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( STR( fd.args[ 0 ] )->get().size() );
}

var_base_t * str_empty( vm_state_t & vm, const fn_data_t & fd )
{
	return STR( fd.args[ 0 ] )->get().size() == 0 ? vm.tru : vm.fals;
}

var_base_t * str_split( vm_state_t & vm, const fn_data_t & fd )
{
	var_str_t * str = STR( fd.args[ 0 ] );
	char delim = ':';
	if( fd.args.size() > 1 ) {
		if( fd.args[ 1 ]->type() != VT_STR ) {
			vm.src_stack.back()->src()->fail( fd.idx, "expected string argument for delimiter, found: %s",
							  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
			return nullptr;
		}
		if( STR( fd.args[ 1 ] )->get().size() > 0 ) STR( fd.args[ 1 ] )->get()[ 0 ];
	}
	std::vector< var_base_t * > res_vec = _str_split( str->get(), delim, fd.src_id, fd.src_id );
	return make< var_vec_t >( res_vec );
}

REGISTER_MODULE( str )
{
	var_src_t * src = vm.src_stack.back();
	const std::string & src_name = src->src()->path();

	vm.add_typefn( VT_STR,   "len", new var_fn_t( src_name, "",  "", {}, { .native = str_size  }, true, 0, 0 ), false );
	vm.add_typefn( VT_STR, "empty", new var_fn_t( src_name, "",  "", {}, { .native = str_empty }, true, 0, 0 ), false );
	vm.add_typefn( VT_STR, "split", new var_fn_t( src_name, "", ".", {}, { .native = str_split }, true, 0, 0 ), false );

	return true;
}


std::vector< var_base_t * > _str_split( const std::string & data, const char delim,
					const size_t & src_id, const size_t & idx )
{
	std::string temp;
	std::vector< var_base_t * > vec;

	for( auto c : data ) {
		if( c == delim ) {
			if( temp.empty() ) continue;
			vec.push_back( new var_str_t( temp, src_id, idx ) );
			temp.clear();
			continue;
		}

		temp += c;
	}

	if( !temp.empty() ) vec.push_back( new var_str_t( temp, src_id, idx ) );
	return vec;
}