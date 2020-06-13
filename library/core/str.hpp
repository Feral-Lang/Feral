/*
	MIT License

	Copyright (c) 2020 Feral Language repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#ifndef LIBRARY_CORE_STR_HPP
#define LIBRARY_CORE_STR_HPP

#include "VM/VM.hpp"

var_base_t * str_add( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected string argument for addition, found: %s",
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
		vm.fail( fd.src_id, fd.idx, "expected integer argument for string multiplication, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	std::string & lhs = STR( fd.args[ 0 ] )->get();
	mpz_t i;
	mpz_init_set_si( i, 0 );
	mpz_t & rhs = INT( fd.args[ 1 ] )->get();
	std::string res;
	for( ; mpz_cmp( i, rhs ) < 0; mpz_add_ui( i, i, 1 ) ) {
		res += lhs;
	}
	mpz_clear( i );
	return make< var_str_t >( res );
}

var_base_t * str_addassn( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected string argument for addition assignment, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	STR( fd.args[ 0 ] )->get() += STR( fd.args[ 1 ] )->get();
	return fd.args[ 0 ];
}

var_base_t * str_mulassn( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected integer argument for string multiplication assignment, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	std::string & lhs = STR( fd.args[ 0 ] )->get();
	mpz_t i;
	mpz_init_set_si( i, 0 );
	mpz_t & rhs = INT( fd.args[ 1 ] )->get();
	std::string res;
	for( ; mpz_cmp( i, rhs ) < 0; mpz_add_ui( i, i, 1 ) ) {
		res += lhs;
	}
	lhs = res;
	mpz_clear( i );
	return fd.args[ 0 ];
}

// logical functions

var_base_t * str_lt( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected string argument for logical less than, found: %s",
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
		vm.fail( fd.src_id, fd.idx, "expected string argument for logical greater than, found: %s",
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
		vm.fail( fd.src_id, fd.idx, "expected string argument for logical less than or equal, found: %s",
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
		vm.fail( fd.src_id, fd.idx, "expected string argument for logical greater than or equal, found: %s",
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
		vm.fail( fd.src_id, fd.idx, "expected argument to be of type integer for string.erase(), found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	std::string & str = STR( fd.args[ 0 ] )->get();
	size_t pos = mpz_get_ui( INT( fd.args[ 1 ] )->get() );
	if( pos >= str.size() ) return vm.nil;
	return make< var_str_t >( std::string( 1, str[ pos ] ) );
}

#endif // LIBRARY_CORE_STR_HPP