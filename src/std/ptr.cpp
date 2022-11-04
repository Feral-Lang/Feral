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

#include "std/ptr_type.hpp"
#include "VM/VM.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

var_base_t *ptr_new_native(vm_state_t &vm, const fn_data_t &fd)
{
	return make<var_ptr_t>(fd.args[1]);
}

var_base_t *ptr_set(vm_state_t &vm, const fn_data_t &fd)
{
	var_ptr_t *self = PTR(fd.args[0]);
	self->update(fd.args[1]);
	return fd.args[0];
}

var_base_t *ptr_get(vm_state_t &vm, const fn_data_t &fd) { return PTR(fd.args[0])->get(); }

INIT_MODULE(ptr)
{
	var_src_t *src = vm.current_source();
	src->add_native_fn("new_native", ptr_new_native, 1);

	vm.add_native_typefn<var_ptr_t>("set", ptr_set, 1, src_id, idx);
	vm.add_native_typefn<var_ptr_t>("get", ptr_get, 0, src_id, idx);
	return true;
}