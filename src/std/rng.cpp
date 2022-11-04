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

#include <random>

#include "VM/VM.hpp"

gmp_randstate_t rngstate;

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

var_base_t *rng_seed(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_int_t>()) {
		vm.fail(fd.src_id, fd.idx, "expected seed value to be integer, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}
	gmp_randseed(rngstate, INT(fd.args[1])->get());
	return vm.nil;
}

// [0, to)
var_base_t *rng_get(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_int_t>()) {
		vm.fail(fd.src_id, fd.idx, "expected upper bound to be an integer, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}
	var_int_t *res = make<var_int_t>(0);
	mpz_urandomm(res->get(), rngstate, INT(fd.args[1])->get());
	return res;
}

INIT_MODULE(rng)
{
	gmp_randinit_default(rngstate);

	var_src_t *src = vm.current_source();

	src->add_native_fn("seed", rng_seed, 1);
	src->add_native_fn("get_native", rng_get, 1);

	return true;
}

DEINIT_MODULE(rng) { gmp_randclear(rngstate); }