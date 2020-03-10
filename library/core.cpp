/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "core/to_str.hpp"
#include "core/int.hpp"

#include "../src/VM/VM.hpp"

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
	// load_fmod() also adds the src to all_srcs map (push_src() function)
	int err = vm.load_fmod( file );
	if( err != E_OK ) {
		src->fail( file_var->idx(), "module import failed, look at error above (exit code: %d)", err );
		return nullptr;
	}
	return vm.all_srcs[ file ];
}
/*
var_base_t * range( vm_state_t & vm, const fn_data_t & fd )
{
	var_base_t * lhs_base = fd.args[ 1 ];
	var_base_t * rhs_base = fd.args.size() > 2 ? fd.args[ 2 ] : nullptr;
	var_base_t * step_base = fd.args.size() > 3 ? fd.args[ 3 ] : nullptr;

	size_t final_type = VT_INT;

	if( lhs_base->type() == VT_FLT ) final_type = VT_FLT;
	if( rhs_base && rhs_base->type() == VT_FLT ) final_type = VT_FLT;
	if( step_base && step_base->type() == VT_FLT ) final_type = VT_FLT;

	if( final_type == VT_INT ) {
		mpz_class begin = fd.args.size() > 2 ? INT( lhs_base )->get() : 0;
		mpz_class end = rhs_base ? INT( rhs_base )->get() : INT( lhs_base )->get();
		mpz_class step = step_base ? INT( step_base )->get() : 1;
		return make< var_int_iterable_t >( begin, end, step );
	}
}
*/
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
	vm.add_typefn( VT_VEC,  "str", new var_fn_t( src_name, {}, { .native = vec_to_str },  0, 0 ), false );
	vm.add_typefn( VT_MAP,  "str", new var_fn_t( src_name, {}, { .native = map_to_str },  0, 0 ), false );

	vm.add_typefn( VT_STRUCT_DEF, "set_typename", new var_fn_t( src_name, { "" }, { .native = struct_def_set_typename },  0, 0 ), false );

	// global required
	vm.gadd( "mload", new var_fn_t( src_name, { "" }, { .native = load_module }, 0, 0 ) );
	vm.gadd( "import", new var_fn_t( src_name, { "" }, { .native = import_file }, 0, 0 ) );
	// vm.gadd( "range", new var_fn_t( src_name, "", ".", { "" }, { .native = range }, true, 0, 0 ) );

	// core type functions
	vm.add_typefn( VT_INT, "+", new var_fn_t( src_name, { "" }, { .native = int_add }, 0, 0 ), false );
	vm.add_typefn( VT_INT, "-", new var_fn_t( src_name, { "" }, { .native = int_sub }, 0, 0 ), false );
	vm.add_typefn( VT_INT, "*", new var_fn_t( src_name, { "" }, { .native = int_mul }, 0, 0 ), false );
	vm.add_typefn( VT_INT, "/", new var_fn_t( src_name, { "" }, { .native = int_div }, 0, 0 ), false );
	vm.add_typefn( VT_INT, "%", new var_fn_t( src_name, { "" }, { .native = int_mod }, 0, 0 ), false );

	vm.add_typefn( VT_INT, "+=", new var_fn_t( src_name, { "" }, { .native = int_addassn }, 0, 0 ), false );
	vm.add_typefn( VT_INT, "-=", new var_fn_t( src_name, { "" }, { .native = int_subassn }, 0, 0 ), false );
	vm.add_typefn( VT_INT, "*=", new var_fn_t( src_name, { "" }, { .native = int_mulassn }, 0, 0 ), false );
	vm.add_typefn( VT_INT, "/=", new var_fn_t( src_name, { "" }, { .native = int_divassn }, 0, 0 ), false );
	vm.add_typefn( VT_INT, "%=", new var_fn_t( src_name, { "" }, { .native = int_modassn }, 0, 0 ), false );

	vm.add_typefn( VT_INT, "**", new var_fn_t( src_name, { "" }, { .native = int_pow },  0, 0 ), false );
	vm.add_typefn( VT_INT, "++x", new var_fn_t( src_name, {}, { .native = int_preinc },  0, 0 ), false );
	vm.add_typefn( VT_INT, "x++", new var_fn_t( src_name, {}, { .native = int_postinc }, 0, 0 ), false );

	vm.add_typefn( VT_INT, "u-", new var_fn_t( src_name, {}, { .native = int_usub }, 0, 0 ), false );

	vm.add_typefn( VT_INT, "<",  new var_fn_t( src_name, { "" }, { .native = int_lt }, 0, 0 ), false );
	vm.add_typefn( VT_INT, ">",  new var_fn_t( src_name, { "" }, { .native = int_gt }, 0, 0 ), false );
	vm.add_typefn( VT_INT, "<=", new var_fn_t( src_name, { "" }, { .native = int_le }, 0, 0 ), false );
	vm.add_typefn( VT_INT, ">=", new var_fn_t( src_name, { "" }, { .native = int_ge }, 0, 0 ), false );
	vm.add_typefn( VT_INT, "==", new var_fn_t( src_name, { "" }, { .native = int_eq }, 0, 0 ), false );
	vm.add_typefn( VT_INT, "!=", new var_fn_t( src_name, { "" }, { .native = int_ne }, 0, 0 ), false );

	return true;
}
