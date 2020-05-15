/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "core/nil.hpp"
#include "core/bool.hpp"
#include "core/int.hpp"
#include "core/flt.hpp"
#include "core/str.hpp"

#include "core/to_str.hpp"

#include "VM/VM.hpp"

var_base_t * all_get_type( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( fd.args[ 0 ]->type() );
}

var_base_t * all_get_typestr( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_str_t >( vm.type_name( fd.args[ 0 ] ) );
}

var_base_t * all_eq( vm_state_t & vm, const fn_data_t & fd )
{
	return fd.args[ 0 ]->type() == fd.args[ 1 ]->type() ? vm.tru : vm.fals;
}

var_base_t * all_ne( vm_state_t & vm, const fn_data_t & fd )
{
	return fd.args[ 0 ]->type() != fd.args[ 1 ]->type() ? vm.tru : vm.fals;
}

var_base_t * all_copy( vm_state_t & vm, const fn_data_t & fd )
{
	var_base_t * copy = fd.args[ 0 ]->copy( fd.src_id, fd.idx );
	// decreased because system internally will increment it again
	copy->dref();
	return copy;
}

var_base_t * reference( vm_state_t & vm, const fn_data_t & fd )
{
	fd.args[ 1 ]->set_load_as_ref();
	return fd.args[ 1 ];
}

var_base_t * load_module( vm_state_t & vm, const fn_data_t & fd )
{
	var_base_t * mod_var = fd.args[ 1 ];
	if( !mod_var->istype< var_str_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected argument to be of type string, found: %s",
			 vm.type_name( mod_var ).c_str() );
		return nullptr;
	}
	std::string mod = STR( mod_var )->get();
	if( !vm.nmod_load( STR( mod_var )->get(), fd.src_id, fd.idx ) ) {
		vm.fail( fd.src_id, fd.idx, "module load failed, look at error above" );
		return nullptr;
	}
	return vm.nil;
}

var_base_t * import_file( vm_state_t & vm, const fn_data_t & fd )
{
	var_base_t * file_var = fd.args[ 1 ];
	if( !file_var->istype< var_str_t >() ) {
		vm.fail( file_var->src_id(), file_var->idx(), "expected argument to be of type string, found: %s",
			 vm.type_name( file_var ).c_str() );
		return nullptr;
	}
	std::string file = STR( file_var )->get();
	if( !vm.mod_exists( vm.inc_locs(), file, fmod_ext() ) ) {
		vm.fail( file_var->src_id(), file_var->idx(), "could not find module file: '%s.fer'", file.c_str() );
		return nullptr;
	}
	// load_fmod() also adds the src to all_srcs map (push_src() function)
	int err = vm.fmod_load( file );
	if( err != E_OK ) {
		vm.fail( file_var->src_id(), file_var->idx(), "module import failed, look at error above (exit code: %d)", err );
		return nullptr;
	}
	return vm.all_srcs[ file ];
}

var_base_t * is_main_src( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.current_source_file();
	return src->is_main() ? vm.tru : vm.fals;
}

INIT_MODULE( core )
{
	const std::string & src_name = vm.current_source_file()->path();

	// fundamental functions for builtin types
	vm.add_native_typefn< var_all_t >( "_type_",     all_get_type,	  0, src_id, idx );
	vm.add_native_typefn< var_all_t >( "_typestr_",  all_get_typestr, 0, src_id, idx );
	vm.add_native_typefn< var_all_t >(     "==",     all_eq,       	  1, src_id, idx );
	vm.add_native_typefn< var_all_t >(     "!=",     all_ne,       	  1, src_id, idx );
	vm.add_native_typefn< var_all_t >(   "copy",     all_copy,     	  0, src_id, idx );

	vm.add_native_typefn< var_all_t >(    "str",     all_to_str,   	  0, src_id, idx );

	vm.add_native_typefn< var_nil_t >(    "str", nil_to_str,    0, src_id, idx );
	vm.add_native_typefn< var_typeid_t >( "str", typeid_to_str, 0, src_id, idx );
	vm.add_native_typefn< var_bool_t >(   "str", bool_to_str,   0, src_id, idx );
	vm.add_native_typefn< var_int_t >(    "str", int_to_str,    0, src_id, idx );
	vm.add_native_typefn< var_flt_t >(    "str", flt_to_str,    0, src_id, idx );
	vm.add_native_typefn< var_str_t >(    "str", str_to_str,    0, src_id, idx );
	vm.add_native_typefn< var_vec_t >(    "str", vec_to_str,    0, src_id, idx );
	vm.add_native_typefn< var_map_t >(    "str", map_to_str,    0, src_id, idx );

	// global required
	vm.gadd( "mload",  new var_fn_t( src_name, { "" }, {}, { .native = load_module }, src_id, idx ), false );
	vm.gadd( "import", new var_fn_t( src_name, { "" }, {}, { .native = import_file }, src_id, idx ), false );
	vm.gadd( "ref",    new var_fn_t( src_name, { "" }, {}, { .native = reference }, src_id, idx ), false );
	vm.gadd( "__ismainsrc__", new var_fn_t( src_name, {}, {}, { .native = is_main_src }, src_id, idx ), false );

	// core type functions

	// nil
	vm.add_native_typefn< var_nil_t >( "==", nil_eq, 1, src_id, idx );
	vm.add_native_typefn< var_nil_t >( "!=", nil_ne, 1, src_id, idx );

	// bool
	vm.add_native_typefn< var_bool_t >( "<",  bool_lt, 1, src_id, idx );
	vm.add_native_typefn< var_bool_t >( ">",  bool_gt, 1, src_id, idx );
	vm.add_native_typefn< var_bool_t >( "<=", bool_le, 1, src_id, idx );
	vm.add_native_typefn< var_bool_t >( ">=", bool_ge, 1, src_id, idx );
	vm.add_native_typefn< var_bool_t >( "==", bool_eq, 1, src_id, idx );
	vm.add_native_typefn< var_bool_t >( "!=", bool_ne, 1, src_id, idx );

	vm.add_native_typefn< var_bool_t >( "!", bool_not, 0, src_id, idx );

	// int
	vm.add_native_typefn< var_int_t >( "+", int_add, 1, src_id, idx );
	vm.add_native_typefn< var_int_t >( "-", int_sub, 1, src_id, idx );
	vm.add_native_typefn< var_int_t >( "*", int_mul, 1, src_id, idx );
	vm.add_native_typefn< var_int_t >( "/", int_div, 1, src_id, idx );
	vm.add_native_typefn< var_int_t >( "%", int_mod, 1, src_id, idx );

	vm.add_native_typefn< var_int_t >( "+=", int_addassn, 1, src_id, idx );
	vm.add_native_typefn< var_int_t >( "-=", int_subassn, 1, src_id, idx );
	vm.add_native_typefn< var_int_t >( "*=", int_mulassn, 1, src_id, idx );
	vm.add_native_typefn< var_int_t >( "/=", int_divassn, 1, src_id, idx );
	vm.add_native_typefn< var_int_t >( "%=", int_modassn, 1, src_id, idx );

	vm.add_native_typefn< var_int_t >( "**",  int_pow, 1, src_id, idx );
	vm.add_native_typefn< var_int_t >( "++x", int_preinc, 0, src_id, idx );
	vm.add_native_typefn< var_int_t >( "x++", int_postinc, 0, src_id, idx );
	vm.add_native_typefn< var_int_t >( "--x", int_predec, 0, src_id, idx );
	vm.add_native_typefn< var_int_t >( "x--", int_postdec, 0, src_id, idx );

	vm.add_native_typefn< var_int_t >( "u-", int_usub, 0, src_id, idx );

	vm.add_native_typefn< var_int_t >( "<",  int_lt, 1, src_id, idx );
	vm.add_native_typefn< var_int_t >( ">",  int_gt, 1, src_id, idx );
	vm.add_native_typefn< var_int_t >( "<=", int_le, 1, src_id, idx );
	vm.add_native_typefn< var_int_t >( ">=", int_ge, 1, src_id, idx );
	vm.add_native_typefn< var_int_t >( "==", int_eq, 1, src_id, idx );
	vm.add_native_typefn< var_int_t >( "!=", int_ne, 1, src_id, idx );

	// flt
	vm.add_native_typefn< var_flt_t >( "+", flt_add, 1, src_id, idx );
	vm.add_native_typefn< var_flt_t >( "-", flt_sub, 1, src_id, idx );
	vm.add_native_typefn< var_flt_t >( "*", flt_mul, 1, src_id, idx );
	vm.add_native_typefn< var_flt_t >( "/", flt_div, 1, src_id, idx );

	vm.add_native_typefn< var_flt_t >( "+=", flt_addassn, 1, src_id, idx );
	vm.add_native_typefn< var_flt_t >( "-=", flt_subassn, 1, src_id, idx );
	vm.add_native_typefn< var_flt_t >( "*=", flt_mulassn, 1, src_id, idx );
	vm.add_native_typefn< var_flt_t >( "/=", flt_divassn, 1, src_id, idx );

	vm.add_native_typefn< var_flt_t >( "++x", flt_preinc, 0, src_id, idx );
	vm.add_native_typefn< var_flt_t >( "x++", flt_postinc, 0, src_id, idx );
	vm.add_native_typefn< var_flt_t >( "--x", flt_predec, 0, src_id, idx );
	vm.add_native_typefn< var_flt_t >( "x--", flt_postdec, 0, src_id, idx );

	vm.add_native_typefn< var_flt_t >( "u-", flt_usub, 0, src_id, idx );

	vm.add_native_typefn< var_flt_t >( "round", flt_round, 0, src_id, idx );

	vm.add_native_typefn< var_flt_t >( "<",  flt_lt, 1, src_id, idx );
	vm.add_native_typefn< var_flt_t >( ">",  flt_gt, 1, src_id, idx );
	vm.add_native_typefn< var_flt_t >( "<=", flt_le, 1, src_id, idx );
	vm.add_native_typefn< var_flt_t >( ">=", flt_ge, 1, src_id, idx );
	vm.add_native_typefn< var_flt_t >( "==", flt_eq, 1, src_id, idx );
	vm.add_native_typefn< var_flt_t >( "!=", flt_ne, 1, src_id, idx );

	// string
	vm.add_native_typefn< var_str_t >( "+", str_add, 1, src_id, idx );
	vm.add_native_typefn< var_str_t >( "*", str_mul, 1, src_id, idx );

	vm.add_native_typefn< var_str_t >( "+=", str_addassn, 1, src_id, idx );
	vm.add_native_typefn< var_str_t >( "*=", str_mulassn, 1, src_id, idx );

	vm.add_native_typefn< var_str_t >( "<",  str_lt, 1, src_id, idx );
	vm.add_native_typefn< var_str_t >( ">",  str_gt, 1, src_id, idx );
	vm.add_native_typefn< var_str_t >( "<=", str_le, 1, src_id, idx );
	vm.add_native_typefn< var_str_t >( ">=", str_ge, 1, src_id, idx );
	vm.add_native_typefn< var_str_t >( "==", str_eq, 1, src_id, idx );
	vm.add_native_typefn< var_str_t >( "!=", str_ne, 1, src_id, idx );

	vm.add_native_typefn< var_str_t >( "at", str_at, 1, src_id, idx );
	vm.add_native_typefn< var_str_t >( "[]", str_at, 1, src_id, idx );

	return true;
}
