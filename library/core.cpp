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

#include "core/int.hpp"

#include "../src/VM/VM.hpp"

var_base_t * all_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.src_stack.back()->src();
	var_base_t * data = fd.args[ 0 ];
	char res[ 1024 ];
	sprintf( res, "type: %s at %p", vm.type_name( data->type() ).c_str(), data );
	return make< var_str_t >( res );
}

var_base_t * nil_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_str_t >( "(nil)" );
}

var_base_t * bool_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_str_t >( BOOL( fd.args[ 0 ] )->get() ? "true" : "false" );
}

var_base_t * int_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_str_t >( INT( fd.args[ 0 ] )->get().get_str() );
}

var_base_t * flt_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	std::ostringstream oss;
	oss << std::setprecision( 21 ) << FLT( fd.args[ 0 ] )->get();
	return make< var_str_t >( oss.str() );
}

var_base_t * str_to_str( vm_state_t & vm, const fn_data_t & fd )
{
	return fd.args[ 0 ];
}

var_base_t * struct_def_set_typename( vm_state_t & vm, const fn_data_t & fd )
{
	if( fd.args[ 1 ]->type() != VT_STR ) {
		vm.src_stack.back()->src()->fail( fd.idx, "expected string argument for typename, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	vm.set_typename( STRUCT_DEF( fd.args[ 0 ] )->id(), STR( fd.args[ 1 ] )->get() );
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
	const std::string & src_name = vm.src_stack.back()->src()->path();

	// fundamental functions for builtin types
	vm.add_typefn( VT_ALL,	"str", new var_fn_t( src_name, {}, { .native = all_to_str },  0, 0 ), false );
	vm.add_typefn( VT_NIL,	"str", new var_fn_t( src_name, {}, { .native = nil_to_str },  0, 0 ), false );
	vm.add_typefn( VT_BOOL,	"str", new var_fn_t( src_name, {}, { .native = bool_to_str }, 0, 0 ), false );
	vm.add_typefn( VT_INT,	"str", new var_fn_t( src_name, {}, { .native = int_to_str },  0, 0 ), false );
	vm.add_typefn( VT_FLT,	"str", new var_fn_t( src_name, {}, { .native = flt_to_str },  0, 0 ), false );
	vm.add_typefn( VT_STR,	"str", new var_fn_t( src_name, {}, { .native = str_to_str },  0, 0 ), false );

	vm.add_typefn( VT_STRUCT_DEF, "set_typename", new var_fn_t( src_name, { "" }, { .native = struct_def_set_typename },  0, 0 ), false );

	// global required
	vm.gadd( "mload", new var_fn_t( src_name, { "" }, { .native = load_module }, 0, 0 ) );
	vm.gadd( "import", new var_fn_t( src_name, { "" }, { .native = import_file }, 0, 0 ) );

	// core type functions
	vm.add_typefn( VT_INT, "+", new var_fn_t( src_name, { "" }, { .native = int_add }, 0, 0 ), false );
	vm.add_typefn( VT_INT, "-", new var_fn_t( src_name, { "" }, { .native = int_sub }, 0, 0 ), false );
	vm.add_typefn( VT_INT, "*", new var_fn_t( src_name, { "" }, { .native = int_mul }, 0, 0 ), false );
	vm.add_typefn( VT_INT, "/", new var_fn_t( src_name, { "" }, { .native = int_div }, 0, 0 ), false );
	vm.add_typefn( VT_INT, "%", new var_fn_t( src_name, { "" }, { .native = int_mod }, 0, 0 ), false );

	vm.add_typefn( VT_INT, "u-", new var_fn_t( src_name, {}, { .native = int_usub }, 0, 0 ), false );

	return true;
}
