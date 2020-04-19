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

#include "../src/VM/VM.hpp"

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

var_base_t * struct_def_set_typename( vm_state_t & vm, const fn_data_t & fd )
{
	if( fd.args[ 1 ]->type() != VT_STR ) {
		vm.current_source_file()->fail( fd.idx, "expected string argument for typename, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	vm.set_typename( STRUCT_DEF( fd.args[ 0 ] )->id(), STR( fd.args[ 1 ] )->get() );
	return vm.nil;
}

var_base_t * load_module( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.current_source_file();
	var_base_t * mod_var = fd.args[ 1 ];
	if( mod_var->type() != VT_STR ) {
		src->fail( fd.idx, "expected argument to be of type string, found: %zu", mod_var->type() );
		return nullptr;
	}
	std::string mod = STR( mod_var )->get();
	if( !vm.load_nmod( STR( mod_var )->get(), fd.src_id, fd.idx ) ) {
		src->fail( fd.idx, "module load failed, look at error above" );
		return nullptr;
	}
	return vm.nil;
}

var_base_t * import_file( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.current_source_file();
	var_base_t * file_var = fd.args[ 1 ];
	if( file_var->type() != VT_STR ) {
		src->fail( file_var->idx(), "expected argument to be of type string, found: %zu", file_var->type() );
		return nullptr;
	}
	std::string file = STR( file_var )->get();
	if( !vm.mod_exists( vm.inc_locs(), file, fmod_ext() ) ) {
		src->fail( file_var->idx(), "could not find module file: '%s.fer'", file.c_str() );
		return nullptr;
	}
	// load_fmod() also adds the src to all_srcs map (push_src() function)
	int err = vm.load_fmod( file );
	if( err != E_OK ) {
		src->fail( file_var->idx(), "module import failed, look at error above (exit code: %d)", err );
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
	vm.add_typefn_native( VT_ALL,	  "==", all_eq,        1, src_id, idx );
	vm.add_typefn_native( VT_ALL,	  "!=", all_ne,        1, src_id, idx );
	vm.add_typefn_native( VT_ALL,	"copy", all_copy,      0, src_id, idx );
	vm.add_typefn_native( VT_ALL,	 "str", all_to_str,    0, src_id, idx );
	vm.add_typefn_native( VT_NIL,	 "str", nil_to_str,    0, src_id, idx );
	vm.add_typefn_native( VT_TYPEID, "str", typeid_to_str, 0, src_id, idx );
	vm.add_typefn_native( VT_BOOL,	 "str", bool_to_str,   0, src_id, idx );
	vm.add_typefn_native( VT_INT,	 "str", int_to_str,    0, src_id, idx );
	vm.add_typefn_native( VT_FLT,	 "str", flt_to_str,    0, src_id, idx );
	vm.add_typefn_native( VT_STR,	 "str", str_to_str,    0, src_id, idx );
	vm.add_typefn_native( VT_VEC,	 "str", vec_to_str,    0, src_id, idx );
	vm.add_typefn_native( VT_MAP,	 "str", map_to_str,    0, src_id, idx );

	vm.add_typefn_native( VT_STRUCT_DEF, "set_typename", struct_def_set_typename, 1, src_id, idx );

	// global required
	vm.gadd( "mload", new var_fn_t( src_name, { "" }, {}, { .native = load_module }, src_id, idx ), false );
	vm.gadd( "import", new var_fn_t( src_name, { "" }, {}, { .native = import_file }, src_id, idx ), false );
	vm.gadd( "__ismainsrc__", new var_fn_t( src_name, {}, {}, { .native = is_main_src }, src_id, idx ), false );

	// core type functions

	// nil
	vm.add_typefn_native( VT_NIL, "==", nil_eq, 1, src_id, idx );
	vm.add_typefn_native( VT_NIL, "!=", nil_ne, 1, src_id, idx );

	// bool
	vm.add_typefn_native( VT_BOOL, "<",  bool_lt, 1, src_id, idx );
	vm.add_typefn_native( VT_BOOL, ">",  bool_gt, 1, src_id, idx );
	vm.add_typefn_native( VT_BOOL, "<=", bool_le, 1, src_id, idx );
	vm.add_typefn_native( VT_BOOL, ">=", bool_ge, 1, src_id, idx );
	vm.add_typefn_native( VT_BOOL, "==", bool_eq, 1, src_id, idx );
	vm.add_typefn_native( VT_BOOL, "!=", bool_ne, 1, src_id, idx );

	vm.add_typefn_native( VT_BOOL, "!", bool_not, 0, src_id, idx );

	// int
	vm.add_typefn_native( VT_INT, "+", int_add, 1, src_id, idx );
	vm.add_typefn_native( VT_INT, "-", int_sub, 1, src_id, idx );
	vm.add_typefn_native( VT_INT, "*", int_mul, 1, src_id, idx );
	vm.add_typefn_native( VT_INT, "/", int_div, 1, src_id, idx );
	vm.add_typefn_native( VT_INT, "%", int_mod, 1, src_id, idx );

	vm.add_typefn_native( VT_INT, "+=", int_addassn, 1, src_id, idx );
	vm.add_typefn_native( VT_INT, "-=", int_subassn, 1, src_id, idx );
	vm.add_typefn_native( VT_INT, "*=", int_mulassn, 1, src_id, idx );
	vm.add_typefn_native( VT_INT, "/=", int_divassn, 1, src_id, idx );
	vm.add_typefn_native( VT_INT, "%=", int_modassn, 1, src_id, idx );

	vm.add_typefn_native( VT_INT, "**",  int_pow, 1, src_id, idx );
	vm.add_typefn_native( VT_INT, "++x", int_preinc, 0, src_id, idx );
	vm.add_typefn_native( VT_INT, "x++", int_postinc, 0, src_id, idx );
	vm.add_typefn_native( VT_INT, "--x", int_predec, 0, src_id, idx );
	vm.add_typefn_native( VT_INT, "x--", int_postdec, 0, src_id, idx );

	vm.add_typefn_native( VT_INT, "u-", int_usub, 0, src_id, idx );

	vm.add_typefn_native( VT_INT, "<",  int_lt, 1, src_id, idx );
	vm.add_typefn_native( VT_INT, ">",  int_gt, 1, src_id, idx );
	vm.add_typefn_native( VT_INT, "<=", int_le, 1, src_id, idx );
	vm.add_typefn_native( VT_INT, ">=", int_ge, 1, src_id, idx );
	vm.add_typefn_native( VT_INT, "==", int_eq, 1, src_id, idx );
	vm.add_typefn_native( VT_INT, "!=", int_ne, 1, src_id, idx );

	// flt
	vm.add_typefn_native( VT_FLT, "+", flt_add, 1, src_id, idx );
	vm.add_typefn_native( VT_FLT, "-", flt_sub, 1, src_id, idx );
	vm.add_typefn_native( VT_FLT, "*", flt_mul, 1, src_id, idx );
	vm.add_typefn_native( VT_FLT, "/", flt_div, 1, src_id, idx );

	vm.add_typefn_native( VT_FLT, "+=", flt_addassn, 1, src_id, idx );
	vm.add_typefn_native( VT_FLT, "-=", flt_subassn, 1, src_id, idx );
	vm.add_typefn_native( VT_FLT, "*=", flt_mulassn, 1, src_id, idx );
	vm.add_typefn_native( VT_FLT, "/=", flt_divassn, 1, src_id, idx );

	vm.add_typefn_native( VT_FLT, "++x", flt_preinc, 0, src_id, idx );
	vm.add_typefn_native( VT_FLT, "x++", flt_postinc, 0, src_id, idx );
	vm.add_typefn_native( VT_FLT, "--x", flt_predec, 0, src_id, idx );
	vm.add_typefn_native( VT_FLT, "x--", flt_postdec, 0, src_id, idx );

	vm.add_typefn_native( VT_FLT, "u-", flt_usub, 0, src_id, idx );

	vm.add_typefn_native( VT_FLT, "round", flt_round, 0, src_id, idx );

	vm.add_typefn_native( VT_FLT, "<",  flt_lt, 1, src_id, idx );
	vm.add_typefn_native( VT_FLT, ">",  flt_gt, 1, src_id, idx );
	vm.add_typefn_native( VT_FLT, "<=", flt_le, 1, src_id, idx );
	vm.add_typefn_native( VT_FLT, ">=", flt_ge, 1, src_id, idx );
	vm.add_typefn_native( VT_FLT, "==", flt_eq, 1, src_id, idx );
	vm.add_typefn_native( VT_FLT, "!=", flt_ne, 1, src_id, idx );

	// string
	vm.add_typefn_native( VT_STR, "+", str_add, 1, src_id, idx );
	vm.add_typefn_native( VT_STR, "*", str_mul, 1, src_id, idx );

	vm.add_typefn_native( VT_STR, "+=", str_addassn, 1, src_id, idx );
	vm.add_typefn_native( VT_STR, "*=", str_mulassn, 1, src_id, idx );

	vm.add_typefn_native( VT_STR, "<",  str_lt, 1, src_id, idx );
	vm.add_typefn_native( VT_STR, ">",  str_gt, 1, src_id, idx );
	vm.add_typefn_native( VT_STR, "<=", str_le, 1, src_id, idx );
	vm.add_typefn_native( VT_STR, ">=", str_ge, 1, src_id, idx );
	vm.add_typefn_native( VT_STR, "==", str_eq, 1, src_id, idx );
	vm.add_typefn_native( VT_STR, "!=", str_ne, 1, src_id, idx );

	vm.add_typefn_native( VT_STR, "at", str_at, 1, src_id, idx );
	vm.add_typefn_native( VT_STR, "[]", str_at, 1, src_id, idx );

	return true;
}