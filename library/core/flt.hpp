/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef LIBRARY_CORE_FLT_HPP
#define LIBRARY_CORE_FLT_HPP

#include "../../src/VM/VM.hpp"

var_base_t * flt_add( vm_state_t & vm, const fn_data_t & fd )
{
	mpfr::mpreal res = FLT( fd.args[ 0 ] )->get();
	mpfr::mpreal rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get().get_str();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for addition, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	res += rhs;
	return make< var_flt_t >( res );
}

var_base_t * flt_sub( vm_state_t & vm, const fn_data_t & fd )
{
	mpfr::mpreal res = FLT( fd.args[ 0 ] )->get();
	mpfr::mpreal rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get().get_str();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for addition, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	res -= rhs;
	return make< var_flt_t >( res );
}

var_base_t * flt_mul( vm_state_t & vm, const fn_data_t & fd )
{
	mpfr::mpreal res = FLT( fd.args[ 0 ] )->get();
	mpfr::mpreal rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get().get_str();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for addition, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	res *= rhs;
	return make< var_flt_t >( res );
}

var_base_t * flt_div( vm_state_t & vm, const fn_data_t & fd )
{
	mpfr::mpreal res = FLT( fd.args[ 0 ] )->get();
	mpfr::mpreal rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get().get_str();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for addition, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( rhs == 0 ) {
		vm.fail( fd.src_id, fd.idx, "modulo with zero as rhs is invalid",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	res /= rhs;
	return make< var_flt_t >( res );
}

var_base_t * flt_addassn( vm_state_t & vm, const fn_data_t & fd )
{
	mpfr::mpreal rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get().get_str();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for addition, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	FLT( fd.args[ 0 ] )->get() += rhs;
	return fd.args[ 0 ];
}

var_base_t * flt_subassn( vm_state_t & vm, const fn_data_t & fd )
{
	mpfr::mpreal rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get().get_str();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for addition, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	FLT( fd.args[ 0 ] )->get() -= rhs;
	return fd.args[ 0 ];
}

var_base_t * flt_mulassn( vm_state_t & vm, const fn_data_t & fd )
{
	mpfr::mpreal rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get().get_str();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for addition, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	FLT( fd.args[ 0 ] )->get() *= rhs;
	return fd.args[ 0 ];
}

var_base_t * flt_divassn( vm_state_t & vm, const fn_data_t & fd )
{
	mpfr::mpreal rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get().get_str();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for addition, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	FLT( fd.args[ 0 ] )->get() /= rhs;
	return fd.args[ 0 ];
}

var_base_t * flt_preinc( vm_state_t & vm, const fn_data_t & fd )
{
	mpfr::mpreal & lhs = FLT( fd.args[ 0 ] )->get();
	++lhs;
	return fd.args[ 0 ];
}

var_base_t * flt_postinc( vm_state_t & vm, const fn_data_t & fd )
{
	mpfr::mpreal & lhs = FLT( fd.args[ 0 ] )->get();
	mpfr::mpreal res = lhs++;
	return make< var_flt_t >( res );
}

var_base_t * flt_predec( vm_state_t & vm, const fn_data_t & fd )
{
	mpfr::mpreal & lhs = FLT( fd.args[ 0 ] )->get();
	--lhs;
	return fd.args[ 0 ];
}

var_base_t * flt_postdec( vm_state_t & vm, const fn_data_t & fd )
{
	mpfr::mpreal & lhs = FLT( fd.args[ 0 ] )->get();
	mpfr::mpreal res = lhs--;
	return make< var_flt_t >( res );
}

var_base_t * flt_usub( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_flt_t >( -FLT( fd.args[ 0 ] )->get() );
}

var_base_t * flt_round( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class res;
	mpfr_get_z( res.get_mpz_t(), FLT( fd.args[ 0 ] )->get().mpfr_ptr(), mpfr_get_default_rounding_mode() );
	return make< var_int_t >( res );
}

// logical functions

var_base_t * flt_lt( vm_state_t & vm, const fn_data_t & fd )
{
	mpfr::mpreal & lhs = FLT( fd.args[ 0 ] )->get();
	mpfr::mpreal rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get().get_str();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for integer less than operation, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	return lhs < rhs ? vm.tru : vm.fals;
}

var_base_t * flt_gt( vm_state_t & vm, const fn_data_t & fd )
{
	mpfr::mpreal & lhs = FLT( fd.args[ 0 ] )->get();
	mpfr::mpreal rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get().get_str();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for integer greater than operation, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	return lhs > rhs ? vm.tru : vm.fals;
}

var_base_t * flt_le( vm_state_t & vm, const fn_data_t & fd )
{
	mpfr::mpreal & lhs = FLT( fd.args[ 0 ] )->get();
	mpfr::mpreal rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get().get_str();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for integer less than/equals operation, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	return lhs <= rhs ? vm.tru : vm.fals;
}

var_base_t * flt_ge( vm_state_t & vm, const fn_data_t & fd )
{
	mpfr::mpreal & lhs = FLT( fd.args[ 0 ] )->get();
	mpfr::mpreal rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get().get_str();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for integer greater than/equals operation, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	return lhs >= rhs ? vm.tru : vm.fals;
}

var_base_t * flt_eq( vm_state_t & vm, const fn_data_t & fd )
{
	mpfr::mpreal & lhs = FLT( fd.args[ 0 ] )->get();
	mpfr::mpreal rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get().get_str();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get();
	} else {
		return vm.fals;
	}
	return lhs == rhs ? vm.tru : vm.fals;
}

var_base_t * flt_ne( vm_state_t & vm, const fn_data_t & fd )
{
	mpfr::mpreal & lhs = FLT( fd.args[ 0 ] )->get();
	mpfr::mpreal rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get().get_str();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get();
	} else {
		return vm.tru;
	}
	return lhs != rhs ? vm.tru : vm.fals;
}

#endif // LIBRARY_CORE_FLT_HPP