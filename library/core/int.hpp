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

#include "../../src/VM/VM.hpp"

var_base_t * int_add( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class res = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->type() == VT_INT ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->type() == VT_FLT ) {
		rhs = mpz_class( mpf_class( FLT( fd.args[ 1 ] )->get().toString() ) );
	} else {
		vm.src_stack.back()->src()->fail( fd.idx, "expected int or float argument for addition, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	res += rhs;
	return make< var_int_t >( res );
}

var_base_t * int_sub( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class res = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->type() == VT_INT ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->type() == VT_FLT ) {
		rhs = mpz_class( mpf_class( FLT( fd.args[ 1 ] )->get().toString() ) );
	} else {
		vm.src_stack.back()->src()->fail( fd.idx, "expected int or float argument for addition, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	res -= rhs;
	return make< var_int_t >( res );
}

var_base_t * int_mul( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class res = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->type() == VT_INT ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->type() == VT_FLT ) {
		rhs = mpz_class( mpf_class( FLT( fd.args[ 1 ] )->get().toString() ) );
	} else {
		vm.src_stack.back()->src()->fail( fd.idx, "expected int or float argument for addition, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	res *= rhs;
	return make< var_int_t >( res );
}

var_base_t * int_div( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class res = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->type() == VT_INT ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->type() == VT_FLT ) {
		rhs = mpz_class( mpf_class( FLT( fd.args[ 1 ] )->get().toString() ) );
	} else {
		vm.src_stack.back()->src()->fail( fd.idx, "expected int or float argument for addition, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	if( rhs == 0 ) {
		vm.src_stack.back()->src()->fail( fd.idx, "modulo with zero as rhs is invalid",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	res /= rhs;
	return make< var_int_t >( res );
}

var_base_t * int_mod( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class res = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->type() == VT_INT ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->type() == VT_FLT ) {
		rhs = mpz_class( mpf_class( FLT( fd.args[ 1 ] )->get().toString() ) );
	} else {
		vm.src_stack.back()->src()->fail( fd.idx, "expected int or float argument for addition, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	if( rhs == 0 ) {
		vm.src_stack.back()->src()->fail( fd.idx, "modulo with zero as rhs is invalid",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	res %= rhs;
	return make< var_int_t >( res );
}

var_base_t * int_usub( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( -INT( fd.args[ 0 ] )->get() );
}
