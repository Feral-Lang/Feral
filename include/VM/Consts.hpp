/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef VM_CONSTS_HPP
#define VM_CONSTS_HPP

#include "Vars/Base.hpp"
#include "OpCodes.hpp"
#include "VM.hpp"

namespace consts
{

var_base_t * get( vm_state_t & vm, const OpDataType type, const op_data_t & opd, const size_t & src_id, const size_t & idx );

}

#endif // VM_CONSTS_HPP
