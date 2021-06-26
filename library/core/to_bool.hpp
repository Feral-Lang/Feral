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

#ifndef LIBRARY_CORE_TO_BOOL_HPP
#define LIBRARY_CORE_TO_BOOL_HPP

#include "VM/VM.hpp"

var_base_t *all_to_bool(vm_state_t &vm, const fn_data_t &fd)
{
	return make<var_bool_t>(false);
}

var_base_t *nil_to_bool(vm_state_t &vm, const fn_data_t &fd)
{
	return make<var_bool_t>(false);
}

var_base_t *bool_to_bool(vm_state_t &vm, const fn_data_t &fd)
{
	return fd.args[0];
}

var_base_t *typeid_to_bool(vm_state_t &vm, const fn_data_t &fd)
{
	return make<var_bool_t>(true);
}

var_base_t *int_to_bool(vm_state_t &vm, const fn_data_t &fd)
{
	return make<var_bool_t>(mpz_cmp_si(INT(fd.args[0])->get(), 0));
}

var_base_t *flt_to_bool(vm_state_t &vm, const fn_data_t &fd)
{
	return make<var_bool_t>(mpfr_cmp_si(FLT(fd.args[0])->get(), 0));
}

var_base_t *str_to_bool(vm_state_t &vm, const fn_data_t &fd)
{
	return make<var_bool_t>(!STR(fd.args[0])->get().empty());
}

var_base_t *vec_to_bool(vm_state_t &vm, const fn_data_t &fd)
{
	return make<var_bool_t>(!VEC(fd.args[0])->get().empty());
}

var_base_t *map_to_bool(vm_state_t &vm, const fn_data_t &fd)
{
	return make<var_bool_t>(!MAP(fd.args[0])->get().empty());
}

#endif // LIBRARY_CORE_TO_BOOL_HPP