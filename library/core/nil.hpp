/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef LIBRARY_CORE_NIL_HPP
#define LIBRARY_CORE_NIL_HPP

#include "VM/VM.hpp"

var_base_t * nil_eq( vm_state_t & vm, const fn_data_t & fd )
{
	return fd.args[ 1 ]->istype< var_nil_t >() ? vm.tru : vm.fals;
}

var_base_t * nil_ne( vm_state_t & vm, const fn_data_t & fd )
{
	return !fd.args[ 1 ]->istype< var_nil_t >() ? vm.tru : vm.fals;
}

#endif // LIBRARY_CORE_NIL_HPP