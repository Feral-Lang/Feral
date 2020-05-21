/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef LIBRARY_CORE_INT_HPP
#define LIBRARY_CORE_INT_HPP

#include "VM/VM.hpp"

var_base_t * int_add( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class res = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for addition, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	res += rhs;
	return make< var_int_t >( res );
}

var_base_t * int_sub( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class res = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for subtraction, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	res -= rhs;
	return make< var_int_t >( res );
}

var_base_t * int_mul( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class res = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for multiplication, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	res *= rhs;
	return make< var_int_t >( res );
}

var_base_t * int_div( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class res = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for division, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( rhs == 0 ) {
		vm.fail( fd.src_id, fd.idx, "modulo with zero as rhs is invalid",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	res /= rhs;
	return make< var_int_t >( res );
}

var_base_t * int_mod( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class res = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for modulo, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( rhs == 0 ) {
		vm.fail( fd.src_id, fd.idx, "modulo with zero as rhs is invalid",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	res %= rhs;
	return make< var_int_t >( res );
}

var_base_t * int_lshift( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class res = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for left shift, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( rhs == 0 ) {
		vm.fail( fd.src_id, fd.idx, "modulo with zero as rhs is invalid",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	res <<= rhs.get_ui();
	return make< var_int_t >( res );
}

var_base_t * int_rshift( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class res = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for right shift, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( rhs == 0 ) {
		vm.fail( fd.src_id, fd.idx, "modulo with zero as rhs is invalid",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	res >>= rhs.get_ui();
	return make< var_int_t >( res );
}

var_base_t * int_addassn( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for addition assign, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	INT( fd.args[ 0 ] )->get() += rhs;
	return fd.args[ 0 ];
}

var_base_t * int_subassn( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for subtraction assign, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	INT( fd.args[ 0 ] )->get() -= rhs;
	return fd.args[ 0 ];
}

var_base_t * int_mulassn( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for multiplication assign, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	INT( fd.args[ 0 ] )->get() *= rhs;
	return fd.args[ 0 ];
}

var_base_t * int_divassn( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for division assign, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	INT( fd.args[ 0 ] )->get() /= rhs;
	return fd.args[ 0 ];
}

var_base_t * int_modassn( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for modulo assign, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	INT( fd.args[ 0 ] )->get() %= rhs;
	return fd.args[ 0 ];
}

var_base_t * int_lshiftassn( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for left shift assign, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	INT( fd.args[ 0 ] )->get() <<= rhs.get_ui();
	return fd.args[ 0 ];
}

var_base_t * int_rshiftassn( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for right shift assign, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	INT( fd.args[ 0 ] )->get() >>= rhs.get_ui();
	return fd.args[ 0 ];
}

var_base_t * int_pow( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class lhs = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for power, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	var_int_t * res = make< var_int_t >( 0 );
	mpz_pow_ui( res->get().get_mpz_t(), lhs.get_mpz_t(), rhs.get_ui() );
	return res;
}

var_base_t * int_preinc( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class & lhs = INT( fd.args[ 0 ] )->get();
	++lhs;
	return fd.args[ 0 ];
}

var_base_t * int_postinc( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class lhs = INT( fd.args[ 0 ] )->get();
	mpz_class res = lhs++;
	return make< var_int_t >( res );
}

var_base_t * int_predec( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class & lhs = INT( fd.args[ 0 ] )->get();
	--lhs;
	return fd.args[ 0 ];
}

var_base_t * int_postdec( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class lhs = INT( fd.args[ 0 ] )->get();
	mpz_class res = lhs--;
	return make< var_int_t >( res );
}

var_base_t * int_usub( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( -INT( fd.args[ 0 ] )->get() );
}

// logical functions

var_base_t * int_lt( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class lhs = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for integer less than operation, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	return lhs < rhs ? vm.tru : vm.fals;
}

var_base_t * int_gt( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class lhs = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for integer greater than operation, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	return lhs > rhs ? vm.tru : vm.fals;
}

var_base_t * int_le( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class lhs = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for integer less than/equals operation, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	return lhs <= rhs ? vm.tru : vm.fals;
}

var_base_t * int_ge( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class lhs = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		vm.fail( fd.src_id, fd.idx, "expected int or float argument for integer greater than/equals operation, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	return lhs >= rhs ? vm.tru : vm.fals;
}

var_base_t * int_eq( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class lhs = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		return vm.fals;
	}
	return lhs == rhs ? vm.tru : vm.fals;
}

var_base_t * int_ne( vm_state_t & vm, const fn_data_t & fd )
{
	mpz_class lhs = INT( fd.args[ 0 ] )->get();
	mpz_class rhs = 0;
	if( fd.args[ 1 ]->istype< var_int_t >() ) {
		rhs = INT( fd.args[ 1 ] )->get();
	} else if( fd.args[ 1 ]->istype< var_flt_t >() ) {
		rhs = FLT( fd.args[ 1 ] )->get().toInt();
	} else {
		return vm.tru;
	}
	return lhs != rhs ? vm.tru : vm.fals;
}

#endif // LIBRARY_CORE_INT_HPP
