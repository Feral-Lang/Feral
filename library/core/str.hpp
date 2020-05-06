/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef LIBRARY_CORE_STR_HPP
#define LIBRARY_CORE_STR_HPP

#include "../../src/VM/VM.hpp"

var_base_t * str_add( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.idx, "expected string argument for addition, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	std::string & lhs = STR( fd.args[ 0 ] )->get();
	std::string & rhs = STR( fd.args[ 1 ] )->get();
	return make< var_str_t >( lhs + rhs );
}

var_base_t * str_mul( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.idx, "expected integer argument for string multiplication, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	std::string & lhs = STR( fd.args[ 0 ] )->get();
	mpz_class & rhs = INT( fd.args[ 1 ] )->get();
	std::string res;
	for( mpz_class i = 0; i < rhs; ++i ) {
		res += lhs;
	}
	return make< var_str_t >( res );
}

var_base_t * str_addassn( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.idx, "expected string argument for addition assignment, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	STR( fd.args[ 0 ] )->get() += STR( fd.args[ 1 ] )->get();
	return fd.args[ 0 ];
}

var_base_t * str_mulassn( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.idx, "expected integer argument for string multiplication assignment, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	std::string & lhs = STR( fd.args[ 0 ] )->get();
	mpz_class & rhs = INT( fd.args[ 1 ] )->get();
	std::string res;
	for( mpz_class i = 0; i < rhs; ++i ) {
		res += lhs;
	}
	lhs = res;
	return fd.args[ 0 ];
}

// logical functions

var_base_t * str_lt( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.idx, "expected string argument for logical less than, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	std::string & lhs = STR( fd.args[ 0 ] )->get();
	std::string & rhs = STR( fd.args[ 1 ] )->get();
	return lhs < rhs ? vm.tru : vm.fals;
}

var_base_t * str_gt( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.idx, "expected string argument for logical greater than, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	std::string & lhs = STR( fd.args[ 0 ] )->get();
	std::string & rhs = STR( fd.args[ 1 ] )->get();
	return lhs > rhs ? vm.tru : vm.fals;
}

var_base_t * str_le( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.idx, "expected string argument for logical less than or equal, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	std::string & lhs = STR( fd.args[ 0 ] )->get();
	std::string & rhs = STR( fd.args[ 1 ] )->get();
	return lhs <= rhs ? vm.tru : vm.fals;
}

var_base_t * str_ge( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.idx, "expected string argument for logical greater than or equal, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	std::string & lhs = STR( fd.args[ 0 ] )->get();
	std::string & rhs = STR( fd.args[ 1 ] )->get();
	return lhs >= rhs ? vm.tru : vm.fals;
}

var_base_t * str_eq( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		return vm.fals;
	}
	std::string & lhs = STR( fd.args[ 0 ] )->get();
	std::string & rhs = STR( fd.args[ 1 ] )->get();
	return lhs == rhs ? vm.tru : vm.fals;
}

var_base_t * str_ne( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		return vm.tru;
	}
	std::string & lhs = STR( fd.args[ 0 ] )->get();
	std::string & rhs = STR( fd.args[ 1 ] )->get();
	return lhs != rhs ? vm.tru : vm.fals;
}

var_base_t * str_at( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.idx, "expected argument to be of type integer for string.erase(), found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	std::string & str = STR( fd.args[ 0 ] )->get();
	size_t pos = INT( fd.args[ 1 ] )->get().get_ui();
	if( pos >= str.size() ) return vm.nil;
	return make< var_str_t >( std::string( 1, str[ pos ] ) );
}

#endif // LIBRARY_CORE_STR_HPP