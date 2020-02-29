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
	char res[ 1024 ];
	sprintf( res, "object at %p", fd.args[ 0 ] );
	return make< var_str_t >( res, fd.idx );
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

var_base_t * mod_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	var_module_t * mod = MOD( fd.args[ 0 ] );
	std::string str = "module id: " + std::to_string( mod->src()->get_id() ) + " (" + mod->src()->get_path() + ")";
	return make< var_str_t >( str, fd.idx );
}

var_base_t * struct_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	var_struct_t * st = STRUCT( fd.args[ 0 ] );
	srcfile_t * src = vm.src_stack.back()->src();

	std::string str = "struct id: " + std::to_string( st->id() ) + " {";
	for( auto & attr : st->attrs() ) {
		var_base_t * str_fn = vm.sattr( attr.second, "str" );
		if( str_fn == nullptr || str_fn->type() != VT_FUNC ) {
			src->fail( attr.second->idx(), "type of this variable does not implement a 'str' function to print" );
			return nullptr;
		}
		if( !FN( str_fn )->call( vm, { attr.second }, {}, fd.idx ) ) {
			src->fail( attr.second->idx(), "failed to call 'str' function (make sure the argument count is correct)" );
			return nullptr;
		}
		var_base_t * res = vm.vm_stack->back();
		if( res->type() != VT_STR ) {
			src->fail( attr.second->idx(), "expected string return type from 'str' function" );
			vm.vm_stack->pop_back();
			return nullptr;
		}
		str += attr.first + ": " + STR( res )->get() + ", ";
	}
	if( st->attrs().size() > 0 ) {
		str.pop_back();
		str.pop_back();
	}
	str += "}";
	return make< var_str_t >( str, fd.idx );
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
		src->fail( fd.idx, "expected argument to be of type string, found: %zu", file_var->type() );
		return nullptr;
	}
	std::string file = STR( file_var )->get();
	if( !vm.mod_exists( vm.inc_locs, file, ".fer" ) ) {
		src->fail( fd.idx, "could not find module file: '%s'", file.c_str() );
		return nullptr;
	}
	// load_fmod() also adds the src to all_srcs map (add_src() function)
	int err = vm.load_fmod( file );
	if( err != E_OK ) {
		src->fail( fd.idx, "module import failed, look at error above (exit code: %d)", err );
		return nullptr;
	}
	return vm.all_srcs[ file ];
}

REGISTER_MODULE( core )
{
	size_t src_id = vm.src_stack.size() - 1;

	// fundamental functions for types
	vm.saddattr( VT_ALL,  "str", new var_fn_t( src_id, { "" }, { .native = all_to_str },  0 ) );
	vm.saddattr( VT_BOOL, "str", new var_fn_t( src_id, { "" }, { .native = bool_to_str }, 0 ) );
	vm.saddattr( VT_INT,  "str", new var_fn_t( src_id, { "" }, { .native = int_to_str },  0 ) );
	vm.saddattr( VT_FLT,  "str", new var_fn_t( src_id, { "" }, { .native = flt_to_str },  0 ) );
	vm.saddattr( VT_STR,  "str", new var_fn_t( src_id, { "" }, { .native = str_to_str },  0 ) );
	vm.saddattr( VT_MOD,  "str", new var_fn_t( src_id, { "" }, { .native = mod_to_str },  0 ) );
	vm.saddattr( VT_STRUCT,  "str", new var_fn_t( src_id, { "" }, { .native = struct_to_str },  0 ) );

	// global required
	vm.gadd( "mload", new var_fn_t( src_id, { "" }, { .native = load_module }, 0 ) );
	vm.gadd( "import", new var_fn_t( src_id, { "" }, { .native = import_file }, 0 ) );
	return true;
}
