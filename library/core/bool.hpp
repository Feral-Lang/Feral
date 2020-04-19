/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef LIBRARY_CORE_BOOL_HPP
#define LIBRARY_CORE_BOOL_HPP

#include "../../src/VM/VM.hpp"

// logical functions

var_base_t * bool_lt( vm_state_t & vm, const fn_data_t & fd )
{
	if( fd.args[ 1 ]->type() != VT_BOOL ) {
		vm.current_source_file()->fail( fd.idx, "expected boolean argument for logical less than, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	return BOOL( fd.args[ 0 ] )->get() < BOOL( fd.args[ 1 ] )->get() ? vm.tru : vm.fals;
}

var_base_t * bool_gt( vm_state_t & vm, const fn_data_t & fd )
{
	if( fd.args[ 1 ]->type() != VT_BOOL ) {
		vm.current_source_file()->fail( fd.idx, "expected boolean argument for logical greater than, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	return BOOL( fd.args[ 0 ] )->get() > BOOL( fd.args[ 1 ] )->get() ? vm.tru : vm.fals;
}

var_base_t * bool_le( vm_state_t & vm, const fn_data_t & fd )
{
	if( fd.args[ 1 ]->type() != VT_BOOL ) {
		vm.current_source_file()->fail( fd.idx, "expected boolean argument for logical less than or equal, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	return BOOL( fd.args[ 0 ] )->get() <= BOOL( fd.args[ 1 ] )->get() ? vm.tru : vm.fals;
}

var_base_t * bool_ge( vm_state_t & vm, const fn_data_t & fd )
{
	if( fd.args[ 1 ]->type() != VT_BOOL ) {
		vm.current_source_file()->fail( fd.idx, "expected boolean argument for logical greater than or equal, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	return BOOL( fd.args[ 0 ] )->get() >= BOOL( fd.args[ 1 ] )->get() ? vm.tru : vm.fals;
}

var_base_t * bool_eq( vm_state_t & vm, const fn_data_t & fd )
{
	if( fd.args[ 1 ]->type() != VT_BOOL ) {
		return vm.fals;
	}
	return BOOL( fd.args[ 0 ] )->get() == BOOL( fd.args[ 1 ] )->get() ? vm.tru : vm.fals;
}

var_base_t * bool_ne( vm_state_t & vm, const fn_data_t & fd )
{
	if( fd.args[ 1 ]->type() != VT_BOOL ) {
		return vm.tru;
	}
	return BOOL( fd.args[ 0 ] )->get() != BOOL( fd.args[ 1 ] )->get() ? vm.tru : vm.fals;
}

var_base_t * bool_not( vm_state_t & vm, const fn_data_t & fd )
{
	return BOOL( fd.args[ 0 ] )->get() ? vm.fals : vm.tru;
}

#endif // LIBRARY_CORE_BOOL_HPP