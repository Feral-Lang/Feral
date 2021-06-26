/*
  Copyright (c) 2020, Electrux
  All rights reserved.
  Using the GNU GPL 3.0 license for the project,
  main LICENSE file resides in project's root directory.
  Please read that file and understand the license terms
  before using or altering the project.
*/

#include "VM/Consts.hpp"

namespace consts
{
var_base_t *get(vm_state_t &vm, const OpDataType type, const op_data_t &opd, const size_t &src_id,
		const size_t &idx)
{
	switch(type) {
	case ODT_BOOL: return opd.b ? vm.tru : vm.fals;
	case ODT_NIL: return vm.nil;
	case ODT_INT: return make_all<var_int_t>(opd.s, src_id, idx);
	case ODT_FLT: return make_all<var_flt_t>(opd.s, src_id, idx);
	case ODT_STR: return make_all<var_str_t>(opd.s, src_id, idx);
	default: break;
	}
	return nullptr;
}

} // namespace consts
