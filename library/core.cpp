/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <iomanip>
#include <sstream>

#include "../src/VM/VM.hpp"

// fmod = feral module
bool fmod_load( vm_state_t & vm, const std::string & mod, const size_t & idx );

var_base_t * all_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.src_stack.back()->src();
	var_base_t * st = fd.args[ 0 ];

	std::string str = "struct " + std::to_string( st->type() ) + " {";
	for( auto & attr : st->attrs() ) {
		if( attr.first == "str" ) continue;
		var_base_t * res = attr.second->call_fn_result( vm, "str", {}, fd.idx );
		if( !res ) return nullptr;
		if( res->type() != VT_STR ) {
			src->fail( attr.second->idx(), "expected string return type from 'str' function, received: %zu", res->type() );
			vm.vm_stack->pop_back();
			return nullptr;
		}
		str += attr.first + ": " + STR( res )->get() + ", ";
		vm.vm_stack->pop_back();
	}
	if( st->attrs().size() > 1 ) {
		str.pop_back();
		str.pop_back();
	}
	str += "}";
	return make< var_str_t >( str, fd.idx );
}

var_base_t * nil_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_str_t >( "(nil)", fd.idx );
}

var_base_t * bool_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_str_t >( BOOL( fd.args[ 0 ] )->get() ? "true" : "false", fd.idx );
}

var_base_t * int_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_str_t >( INT( fd.args[ 0 ] )->get().get_str(), fd.idx );
}

var_base_t * flt_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	std::ostringstream oss;
	oss << std::setprecision( 21 ) << FLT( fd.args[ 0 ] )->get();
	return make< var_str_t >( oss.str(), fd.idx );
}

var_base_t * str_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	return fd.args[ 0 ];
}

var_base_t * vec_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.src_stack.back()->src();
	var_vec_t * vec = VEC( fd.args[ 0 ] );
	std::string str = "[";
	for( auto & e : vec->get() ) {
		var_base_t * res = e->call_fn_result( vm, "str", {}, fd.idx );
		if( !res ) return nullptr;
		if( res->type() != VT_STR ) {
			src->fail( e->idx(), "expected string return type from 'str' function, received: %zu", res->type() );
			vm.vm_stack->pop_back();
			return nullptr;
		}
		str += STR( res )->get() + ", ";
		vm.vm_stack->pop_back();
	}
	if( vec->get().size() > 0 ) {
		str.pop_back();
		str.pop_back();
	}
	str += "]";
	return make< var_str_t >( str, fd.idx );
}

var_base_t * map_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.src_stack.back()->src();
	var_map_t * map = MAP( fd.args[ 0 ] );
	std::string str = "{";
	for( auto & e : map->get() ) {
		if( e.first == "str" ) continue;
		var_base_t * res = e.second->call_fn_result( vm, "str", {}, fd.idx );
		if( !res ) return nullptr;
		if( res->type() != VT_STR ) {
			src->fail( e.second->idx(), "expected string return type from 'str' function, received: %zu", res->type() );
			vm.vm_stack->pop_back();
			return nullptr;
		}
		str += e.first + ": " + STR( res )->get() + ", ";
		vm.vm_stack->pop_back();
	}
	if( map->get().size() > 0 ) {
		str.pop_back();
		str.pop_back();
	}
	str += "}";
	return make< var_str_t >( str, fd.idx );
}

var_base_t * mod_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	var_module_t * mod = MOD( fd.args[ 0 ] );
	std::string str = "module id: " + std::to_string( mod->src()->get_id() ) + " (" + mod->src()->get_path() + ")";
	return make< var_str_t >( str, fd.idx );
}

var_base_t * fuse_custom( vm_state_t & vm, const fn_data_t & fd )
{
	fd.args[ 0 ]->fuse( fd.args[ 1 ] );
	return vm.nil;
}

var_base_t * load_module( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.src_stack.back()->src();
	var_base_t * mod_var = fd.args[ 1 ];
	if( mod_var->type() != VT_STR ) {
		src->fail( fd.idx, "expected argument to be of type string, found: %zu", mod_var->type() );
		return nullptr;
	}
	std::string mod = STR( mod_var )->get();
	if( !vm.load_nmod( STR( mod_var )->get(), fd.idx ) ) {
		src->fail( fd.idx, "module load failed, look at error above" );
		return nullptr;
	}
	return vm.nil;
}

var_base_t * import_file( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.src_stack.back()->src();
	var_base_t * file_var = fd.args[ 1 ];
	if( file_var->type() != VT_STR ) {
		src->fail( file_var->idx(), "expected argument to be of type string, found: %zu", file_var->type() );
		return nullptr;
	}
	std::string file = STR( file_var )->get();
	if( !vm.mod_exists( vm.inc_locs, file, ".fer" ) ) {
		src->fail( file_var->idx(), "could not find module file: '%s'", file.c_str() );
		return nullptr;
	}
	// load_fmod() also adds the src to all_srcs map (add_src() function)
	int err = vm.load_fmod( file );
	if( err != E_OK ) {
		src->fail( file_var->idx(), "module import failed, look at error above (exit code: %d)", err );
		return nullptr;
	}
	return vm.all_srcs[ file ];
}

REGISTER_MODULE( core )
{
	const std::string & src_name = vm.src_stack.back()->src()->get_path();

	// fundamental functions for types
	vm.btatadd( VT_ALL,  "str", new var_fn_t( src_name, { "" }, { .native = all_to_str },  0 ) );
	vm.btatadd( VT_NIL,  "str", new var_fn_t( src_name, { "" }, { .native = nil_to_str },  0 ) );
	vm.btatadd( VT_BOOL, "str", new var_fn_t( src_name, { "" }, { .native = bool_to_str }, 0 ) );
	vm.btatadd( VT_INT,  "str", new var_fn_t( src_name, { "" }, { .native = int_to_str },  0 ) );
	vm.btatadd( VT_FLT,  "str", new var_fn_t( src_name, { "" }, { .native = flt_to_str },  0 ) );
	vm.btatadd( VT_STR,  "str", new var_fn_t( src_name, { "" }, { .native = str_to_str },  0 ) );
	vm.btatadd( VT_VEC,  "str", new var_fn_t( src_name, { "" }, { .native = vec_to_str },  0 ) );
	vm.btatadd( VT_MAP,  "str", new var_fn_t( src_name, { "" }, { .native = map_to_str },  0 ) );
	vm.btatadd( VT_MOD,  "str", new var_fn_t( src_name, { "" }, { .native = mod_to_str },  0 ) );

	vm.btatadd( VT_CUSTOM_START, "fuse", new var_fn_t( src_name, { "" }, { .native = fuse_custom }, 0 ) );

	// global required
	vm.gadd( "mload", new var_fn_t( src_name, { "" }, { .native = load_module }, 0 ) );
	vm.gadd( "import", new var_fn_t( src_name, { "" }, { .native = import_file }, 0 ) );
	return true;
}
