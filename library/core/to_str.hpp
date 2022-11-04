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

#ifndef LIBRARY_CORE_TO_STR_HPP
#define LIBRARY_CORE_TO_STR_HPP

#include <cstring>

#include "VM/VM.hpp"

var_base_t *all_to_str(vm_state_t &vm, const fn_data_t &fd)
{
	var_base_t *_data = fd.args[0];
	char res[1024];
	sprintf(res, "type: %s at %p", vm.type_name(_data).c_str(), _data);
	return make<var_str_t>(res);
}

var_base_t *nil_to_str(vm_state_t &vm, const fn_data_t &fd) { return make<var_str_t>("(nil)"); }

var_base_t *bool_to_str(vm_state_t &vm, const fn_data_t &fd)
{
	return make<var_str_t>(BOOL(fd.args[0])->get() ? "true" : "false");
}

var_base_t *typeid_to_str(vm_state_t &vm, const fn_data_t &fd)
{
	return make<var_str_t>("typeid<" + std::to_string(TYPEID(fd.args[0])->get()) + ">");
}

var_base_t *int_to_str(vm_state_t &vm, const fn_data_t &fd)
{
	typedef void (*gmp_freefunc_t)(void *, size_t);

	char *_res     = mpz_get_str(NULL, 10, INT(fd.args[0])->get());
	var_str_t *res = make<var_str_t>("");
	res->get()     = _res;

	gmp_freefunc_t freefunc;
	mp_get_memory_functions(NULL, NULL, &freefunc);
	freefunc(_res, strlen(_res) + 1);

	return res;
}

var_base_t *flt_to_str(vm_state_t &vm, const fn_data_t &fd)
{
	mpfr_exp_t expo;
	char *_res =
	mpfr_get_str(NULL, &expo, 10, 0, FLT(fd.args[0])->get(), mpfr_get_default_rounding_mode());
	var_str_t *res = make<var_str_t>(_res);
	mpfr_free_str(_res);
	if(res->get().empty() || expo == 0 || expo > 25) return res;
	auto last_zero_from_end = res->get().find_last_of("123456789");
	if(last_zero_from_end != std::string::npos)
		res->get() = res->get().erase(last_zero_from_end + 1);
	if(expo > 0) {
		std::string &str_res = res->get();
		size_t sz	     = str_res.size();
		while(expo > sz) {
			str_res += '0';
		}
		if(res->get()[0] == '-') ++expo;
		res->get().insert(expo, ".");
	} else {
		std::string pre_zero(-expo, '0');
		res->get() = "0." + pre_zero + res->get();
	}
	return res;
}

var_base_t *str_to_str(vm_state_t &vm, const fn_data_t &fd) { return fd.args[0]; }

var_base_t *vec_to_str(vm_state_t &vm, const fn_data_t &fd)
{
	var_vec_t *vec	= VEC(fd.args[0]);
	std::string res = "[";
	for(auto &e : vec->get()) {
		std::string str;
		if(!e->to_str(vm, str, fd.src_id, fd.idx)) {
			return nullptr;
		}
		res += str + ", ";
	}
	if(vec->get().size() > 0) {
		res.pop_back();
		res.pop_back();
	}
	res += "]";
	return make<var_str_t>(res);
}

var_base_t *map_to_str(vm_state_t &vm, const fn_data_t &fd)
{
	var_map_t *map	= MAP(fd.args[0]);
	std::string res = "{";
	for(auto &e : map->get()) {
		std::string str;
		if(!e.second->to_str(vm, str, fd.src_id, fd.idx)) {
			return nullptr;
		}
		res += e.first + ": " + str + ", ";
	}
	if(map->get().size() > 0) {
		res.pop_back();
		res.pop_back();
	}
	res += "}";
	return make<var_str_t>(res);
}

#endif // LIBRARY_CORE_TO_STR_HPP