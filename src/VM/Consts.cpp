/*
  Copyright (c) 2020, Electrux
  All rights reserved.
  Using the BSD 3-Clause license for the project,
  main LICENSE file resides in project's root directory.
  Please read that file and understand the license terms
  before using or altering the project.
*/

#include "Consts.hpp"

namespace consts
{

// TODO: optimize this
var_base_t * get( vm_state_t & vm, const OpDataType type, const op_data_t & opd, const size_t & idx )
{
	if( type == ODT_BOOL ) {
		if( opd.b ) return vm.tru;
		return vm.fals;
	}

	if( type == ODT_NIL ) {
		return vm.nil;
	}

	if( type == ODT_INT ) return new var_int_t( mpz_class( opd.s ), vm.src_stack.back()->src_id(), idx );
	else if( type == ODT_FLT ) return new var_flt_t( mpfr::mpreal( opd.s ), vm.src_stack.back()->src_id(), idx );
	else if( type == ODT_STR ) return new var_str_t( opd.s, vm.src_stack.back()->src_id(), idx );

	return nullptr;
}

}
