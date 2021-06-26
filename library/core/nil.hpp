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

#ifndef LIBRARY_CORE_NIL_HPP
#define LIBRARY_CORE_NIL_HPP

#include "VM/VM.hpp"

var_base_t *nil_eq(vm_state_t &vm, const fn_data_t &fd)
{
	return fd.args[1]->istype<var_nil_t>() ? vm.tru : vm.fals;
}

var_base_t *nil_ne(vm_state_t &vm, const fn_data_t &fd)
{
	return !fd.args[1]->istype<var_nil_t>() ? vm.tru : vm.fals;
}

#endif // LIBRARY_CORE_NIL_HPP