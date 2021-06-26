/*
	MIT License

	Copyright (c) 2021 Feral Language repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "std/mutex_type.hpp"
#include "VM/VM.hpp"

int exec_command(const std::string &cmd);

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

var_base_t *mutex_new(vm_state_t &vm, const fn_data_t &fd)
{
	return make<var_mutex_t>();
}

var_base_t *mutex_lock(vm_state_t &vm, const fn_data_t &fd)
{
	MUTEX(fd.args[0])->get().lock();
	return vm.nil;
}

var_base_t *mutex_unlock(vm_state_t &vm, const fn_data_t &fd)
{
	MUTEX(fd.args[0])->get().unlock();
	return vm.nil;
}

INIT_MODULE(mutex)
{
	var_src_t *src = vm.current_source();

	src->add_native_fn("new", mutex_new, 0);

	vm.add_native_typefn<var_mutex_t>("lock", mutex_lock, 0, src_id, idx);
	vm.add_native_typefn<var_mutex_t>("unlock", mutex_unlock, 0, src_id, idx);
	return true;
}
