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

#ifndef LIBRARY_CORE_TYPEID_HPP
#define LIBRARY_CORE_TYPEID_HPP

#include "VM/VM.hpp"

// logical functions

var_base_t *typeid_eq(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_typeid_t>()) {
		return vm.fals;
	}
	return TYPEID(fd.args[0])->get() == TYPEID(fd.args[1])->get() ? vm.tru : vm.fals;
}

var_base_t *typeid_ne(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_typeid_t>()) {
		return vm.tru;
	}
	return TYPEID(fd.args[0])->get() != TYPEID(fd.args[1])->get() ? vm.tru : vm.fals;
}

#endif // LIBRARY_CORE_TYPEID_HPP