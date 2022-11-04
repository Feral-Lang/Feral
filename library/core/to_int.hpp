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

#ifndef LIBRARY_CORE_TO_INT_HPP
#define LIBRARY_CORE_TO_INT_HPP

#include "VM/VM.hpp"

var_base_t *nil_to_int(vm_state_t &vm, const fn_data_t &fd) { return make<var_int_t>(0); }

var_base_t *bool_to_int(vm_state_t &vm, const fn_data_t &fd)
{
	return make<var_int_t>(BOOL(fd.args[0])->get() ? 1 : 0);
}

var_base_t *typeid_to_int(vm_state_t &vm, const fn_data_t &fd)
{
	return make<var_int_t>(TYPEID(fd.args[0])->get());
}

var_base_t *int_to_int(vm_state_t &vm, const fn_data_t &fd) { return fd.args[0]; }

var_base_t *flt_to_int(vm_state_t &vm, const fn_data_t &fd)
{
	var_int_t *res = make<var_int_t>(0);
	mpfr_get_z(res->get(), FLT(fd.args[0])->get(), mpfr_get_default_rounding_mode());
	return res;
}

var_base_t *str_to_int(vm_state_t &vm, const fn_data_t &fd)
{
	var_int_t *res = make<var_int_t>(0);
	int tmp	       = mpz_set_str(res->get(), STR(fd.args[0])->get().c_str(), 0);
	if(tmp == 0) return res;
	return vm.nil;
}

#endif // LIBRARY_CORE_TO_INT_HPP