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

#ifndef VM_CONSTS_HPP
#define VM_CONSTS_HPP

#include "OpCodes.hpp"
#include "Vars/Base.hpp"
#include "VM.hpp"

namespace consts
{
var_base_t *get(vm_state_t &vm, const OpDataType type, const op_data_t &opd, const size_t &src_id,
		const size_t &idx);
}

#endif // VM_CONSTS_HPP
