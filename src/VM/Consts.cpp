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
		return opd.b ? vm.tru : vm.fals;
	}

	if( type == ODT_NIL ) {
		return vm.nil;
	}

	if( type == ODT_INT ) return make_all< var_int_t >( mpz_class( opd.s ), vm.current_source()->src_id(), idx );
	else if( type == ODT_FLT ) return make_all< var_flt_t >( mpfr::mpreal( opd.s ), vm.current_source()->src_id(), idx );
	else if( type == ODT_STR ) return make_all< var_str_t >( opd.s, vm.current_source()->src_id(), idx );

	return nullptr;
}

}
