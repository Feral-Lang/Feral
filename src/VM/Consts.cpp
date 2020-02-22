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

var_base_t * get( vm_state_t & vm, const OpDataType type, const op_data_t & opd, const size_t & idx )
{
	if( type == ODT_BOOL ) {
		if( opd.b ) return vm.tru;
		return vm.fals;
	}

	if( type == ODT_NIL ) {
		return vm.nil;
	}

	if( type == ODT_INT ) return new var_int_t( mpz_class( opd.s ), idx );
	else if( type == ODT_FLT ) return new var_flt_t( opd.s, idx );
	else if( type == ODT_STR ) return new var_str_t( opd.s, idx );

	return nullptr;
}

}
