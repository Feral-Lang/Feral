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

#ifndef LIBRARY_CORE_INT_HPP
#define LIBRARY_CORE_INT_HPP

#include "VM/VM.hpp"

#define ARITHI_FUNC(name)                                                                          \
	var_base_t *int_##name(vm_state_t &vm, const fn_data_t &fd)                                \
	{                                                                                          \
		if(fd.args[1]->istype<var_int_t>()) {                                              \
			var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());                  \
			mpz_##name(res->get(), res->get(), INT(fd.args[1])->get());                \
			return res;                                                                \
		} else if(fd.args[1]->istype<var_flt_t>()) {                                       \
			var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());                  \
			mpz_t tmp;                                                                 \
			mpz_init(tmp);                                                             \
			mpfr_get_z(tmp, FLT(fd.args[1])->get(), mpfr_get_default_rounding_mode()); \
			mpz_##name(res->get(), res->get(), tmp);                                   \
			mpz_clear(tmp);                                                            \
			return res;                                                                \
		}                                                                                  \
		vm.fail(fd.src_id, fd.idx,                                                         \
			"expected int or float argument for int " STRINGIFY(name) ", found: %s",   \
			vm.type_name(fd.args[1]).c_str());                                         \
		return nullptr;                                                                    \
	}

#define ARITHI_ASSN_FUNC(name)                                                                     \
	var_base_t *int_assn_##name(vm_state_t &vm, const fn_data_t &fd)                           \
	{                                                                                          \
		if(fd.args[1]->istype<var_int_t>()) {                                              \
			mpz_##name(INT(fd.args[0])->get(), INT(fd.args[0])->get(),                 \
				   INT(fd.args[1])->get());                                        \
			return fd.args[0];                                                         \
		} else if(fd.args[1]->istype<var_flt_t>()) {                                       \
			mpz_t tmp;                                                                 \
			mpz_init(tmp);                                                             \
			mpfr_get_z(tmp, FLT(fd.args[1])->get(), mpfr_get_default_rounding_mode()); \
			mpz_##name(INT(fd.args[0])->get(), INT(fd.args[0])->get(), tmp);           \
			mpz_clear(tmp);                                                            \
			return fd.args[0];                                                         \
		}                                                                                  \
		vm.fail(                                                                           \
		fd.src_id, fd.idx,                                                                 \
		"expected int or float argument for int " STRINGIFY(name) "-assign, found: %s",    \
		vm.type_name(fd.args[1]).c_str());                                                 \
		return nullptr;                                                                    \
	}

#define LOGICI_FUNC(name, sym)                                                               \
	var_base_t *int_##name(vm_state_t &vm, const fn_data_t &fd)                          \
	{                                                                                    \
		if(fd.args[1]->istype<var_int_t>()) {                                        \
			return mpz_cmp(INT(fd.args[0])->get(), INT(fd.args[1])->get()) sym 0 \
			       ? vm.tru                                                      \
			       : vm.fals;                                                    \
		}                                                                            \
		vm.fail(fd.src_id, fd.idx,                                                   \
			"expected int argument for int " STRINGIFY(name) ", found: %s",      \
			vm.type_name(fd.args[1]).c_str());                                   \
		return nullptr;                                                              \
	}

ARITHI_FUNC(add)
ARITHI_FUNC(sub)
ARITHI_FUNC(mul)
ARITHI_FUNC(mod)

ARITHI_ASSN_FUNC(add)
ARITHI_ASSN_FUNC(sub)
ARITHI_ASSN_FUNC(mul)
ARITHI_ASSN_FUNC(mod)

LOGICI_FUNC(lt, <)
LOGICI_FUNC(gt, >)
LOGICI_FUNC(le, <=)
LOGICI_FUNC(ge, >=)

var_base_t *int_div(vm_state_t &vm, const fn_data_t &fd)
{
	if(fd.args[1]->istype<var_int_t>()) {
		// rhs == 0
		if(mpz_get_ui(INT(fd.args[1])->get()) == 0) {
			vm.fail(fd.src_id, fd.idx, "division by zero");
			return nullptr;
		}
		var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());
		mpz_div(res->get(), res->get(), INT(fd.args[1])->get());
		return res;
	} else if(fd.args[1]->istype<var_flt_t>()) {
		var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());
		mpz_t tmp;
		mpz_init(tmp);
		mpfr_get_z(tmp, FLT(fd.args[1])->get(), mpfr_get_default_rounding_mode());
		// rhs == 0
		if(mpz_get_ui(tmp) == 0) {
			vm.fail(fd.src_id, fd.idx, "division by zero");
			return nullptr;
		}
		mpz_div(res->get(), res->get(), tmp);
		mpz_clear(tmp);
		return res;
	}
	vm.fail(fd.src_id, fd.idx,
		"expected int or float argument for int " STRINGIFY(name) ", found: %s",
		vm.type_name(fd.args[1]).c_str());
	return nullptr;
}

var_base_t *int_assn_div(vm_state_t &vm, const fn_data_t &fd)
{
	if(fd.args[1]->istype<var_int_t>()) {
		// rhs == 0
		if(mpz_get_ui(INT(fd.args[1])->get()) == 0) {
			vm.fail(fd.src_id, fd.idx, "division by zero");
			return nullptr;
		}
		mpz_div(INT(fd.args[0])->get(), INT(fd.args[0])->get(), INT(fd.args[1])->get());
		return fd.args[0];
	} else if(fd.args[1]->istype<var_flt_t>()) {
		mpz_t tmp;
		mpz_init(tmp);
		mpfr_get_z(tmp, FLT(fd.args[1])->get(), mpfr_get_default_rounding_mode());
		// rhs == 0
		if(mpz_get_ui(tmp) == 0) {
			vm.fail(fd.src_id, fd.idx, "division by zero");
			return nullptr;
		}
		mpz_div(INT(fd.args[0])->get(), INT(fd.args[0])->get(), tmp);
		mpz_clear(tmp);
		return fd.args[0];
	}
	vm.fail(fd.src_id, fd.idx,
		"expected int or float argument for int " STRINGIFY(name) "-assign, found: %s",
		vm.type_name(fd.args[1]).c_str());
	return nullptr;
}

var_base_t *int_band(vm_state_t &vm, const fn_data_t &fd)
{
	if(fd.args[1]->istype<var_int_t>()) {
		var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());
		mpz_and(res->get(), INT(fd.args[0])->get(), INT(fd.args[1])->get());
		return res;
	}
	vm.fail(fd.src_id, fd.idx, "expected int argument for int bitwise and, found: %s",
		vm.type_name(fd.args[1]).c_str());
	return nullptr;
}

var_base_t *int_bor(vm_state_t &vm, const fn_data_t &fd)
{
	if(fd.args[1]->istype<var_int_t>()) {
		var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());
		mpz_ior(res->get(), INT(fd.args[0])->get(), INT(fd.args[1])->get());
		return res;
	}
	vm.fail(fd.src_id, fd.idx, "expected int argument for int bitwise or, found: %s",
		vm.type_name(fd.args[1]).c_str());
	return nullptr;
}

var_base_t *int_bxor(vm_state_t &vm, const fn_data_t &fd)
{
	if(fd.args[1]->istype<var_int_t>()) {
		var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());
		mpz_xor(res->get(), INT(fd.args[0])->get(), INT(fd.args[1])->get());
		return res;
	}
	vm.fail(fd.src_id, fd.idx, "expected int argument for int bitwise xor, found: %s",
		vm.type_name(fd.args[1]).c_str());
	return nullptr;
}

var_base_t *int_bnot(vm_state_t &vm, const fn_data_t &fd)
{
	var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());
	mpz_com(res->get(), INT(fd.args[0])->get());
	return res;
}

var_base_t *int_bandassn(vm_state_t &vm, const fn_data_t &fd)
{
	if(fd.args[1]->istype<var_int_t>()) {
		mpz_and(INT(fd.args[0])->get(), INT(fd.args[0])->get(), INT(fd.args[1])->get());
		return fd.args[0];
	}
	vm.fail(fd.src_id, fd.idx, "expected int argument for int bitwise and-assn, found: %s",
		vm.type_name(fd.args[1]).c_str());
	return nullptr;
}

var_base_t *int_borassn(vm_state_t &vm, const fn_data_t &fd)
{
	if(fd.args[1]->istype<var_int_t>()) {
		mpz_ior(INT(fd.args[0])->get(), INT(fd.args[0])->get(), INT(fd.args[1])->get());
		return fd.args[0];
	}
	vm.fail(fd.src_id, fd.idx, "expected int argument for int bitwise or-assn, found: %s",
		vm.type_name(fd.args[1]).c_str());
	return nullptr;
}

var_base_t *int_bxorassn(vm_state_t &vm, const fn_data_t &fd)
{
	if(fd.args[1]->istype<var_int_t>()) {
		mpz_xor(INT(fd.args[0])->get(), INT(fd.args[0])->get(), INT(fd.args[1])->get());
		return fd.args[0];
	}
	vm.fail(fd.src_id, fd.idx, "expected int argument for int bitwise xor, found: %s",
		vm.type_name(fd.args[1]).c_str());
	return nullptr;
}

var_base_t *int_bnotassn(vm_state_t &vm, const fn_data_t &fd)
{
	mpz_com(INT(fd.args[0])->get(), INT(fd.args[0])->get());
	return fd.args[0];
}

var_base_t *int_popcnt(vm_state_t &vm, const fn_data_t &fd)
{
	return make<var_int_t>(mpz_popcount(INT(fd.args[0])->get()));
}

var_base_t *int_eq(vm_state_t &vm, const fn_data_t &fd)
{
	if(fd.args[1]->istype<var_int_t>()) {
		return mpz_cmp(INT(fd.args[0])->get(), INT(fd.args[1])->get()) == 0 ? vm.tru
										    : vm.fals;
	}
	return vm.fals;
}

var_base_t *int_ne(vm_state_t &vm, const fn_data_t &fd)
{
	if(fd.args[1]->istype<var_int_t>()) {
		return mpz_cmp(INT(fd.args[0])->get(), INT(fd.args[1])->get()) != 0 ? vm.tru
										    : vm.fals;
	}
	return vm.tru;
}

var_base_t *int_lshift(vm_state_t &vm, const fn_data_t &fd)
{
	if(fd.args[1]->istype<var_int_t>()) {
		var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());
		mpz_mul_2exp(res->get(), INT(fd.args[0])->get(),
			     mpz_get_si(INT(fd.args[1])->get()));
		return res;
	} else if(fd.args[1]->istype<var_flt_t>()) {
		var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());
		mpz_t tmp;
		mpz_init(tmp);
		mpfr_get_z(tmp, FLT(fd.args[1])->get(), mpfr_get_default_rounding_mode());
		mpz_mul_2exp(res->get(), INT(fd.args[0])->get(), mpz_get_si(tmp));
		mpz_clear(tmp);
		return res;
	}
	vm.fail(fd.src_id, fd.idx, "expected int or float argument for int leftshift, found: %s",
		vm.type_name(fd.args[1]).c_str());
	return nullptr;
}

var_base_t *int_rshift(vm_state_t &vm, const fn_data_t &fd)
{
	if(fd.args[1]->istype<var_int_t>()) {
		var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());
		mpz_div_2exp(res->get(), INT(fd.args[0])->get(),
			     mpz_get_si(INT(fd.args[1])->get()));
		return res;
	} else if(fd.args[1]->istype<var_flt_t>()) {
		var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());
		mpz_t tmp;
		mpz_init(tmp);
		mpfr_get_z(tmp, FLT(fd.args[1])->get(), mpfr_get_default_rounding_mode());
		mpz_div_2exp(res->get(), INT(fd.args[0])->get(), mpz_get_si(tmp));
		mpz_clear(tmp);
		return res;
	}
	vm.fail(fd.src_id, fd.idx, "expected int or float argument for int rightshift, found: %s",
		vm.type_name(fd.args[1]).c_str());
	return nullptr;
}

var_base_t *int_lshiftassn(vm_state_t &vm, const fn_data_t &fd)
{
	if(fd.args[1]->istype<var_int_t>()) {
		mpz_mul_2exp(INT(fd.args[0])->get(), INT(fd.args[0])->get(),
			     mpz_get_si(INT(fd.args[1])->get()));
		return fd.args[0];
	} else if(fd.args[1]->istype<var_flt_t>()) {
		mpz_t tmp;
		mpz_init(tmp);
		mpfr_get_z(tmp, FLT(fd.args[1])->get(), mpfr_get_default_rounding_mode());
		mpz_mul_2exp(INT(fd.args[0])->get(), INT(fd.args[0])->get(), mpz_get_si(tmp));
		mpz_clear(tmp);
		return fd.args[0];
	}
	vm.fail(fd.src_id, fd.idx,
		"expected int or float argument for int leftshift-assign, found: %s",
		vm.type_name(fd.args[1]).c_str());
	return nullptr;
}

var_base_t *int_rshiftassn(vm_state_t &vm, const fn_data_t &fd)
{
	if(fd.args[1]->istype<var_int_t>()) {
		mpz_div_2exp(INT(fd.args[0])->get(), INT(fd.args[0])->get(),
			     mpz_get_si(INT(fd.args[1])->get()));
		return fd.args[0];
	} else if(fd.args[1]->istype<var_flt_t>()) {
		mpz_t tmp;
		mpz_init(tmp);
		mpfr_get_z(tmp, FLT(fd.args[1])->get(), mpfr_get_default_rounding_mode());
		mpz_div_2exp(INT(fd.args[0])->get(), INT(fd.args[0])->get(), mpz_get_si(tmp));
		mpz_clear(tmp);
		return fd.args[0];
	}
	vm.fail(fd.src_id, fd.idx,
		"expected int or float argument for int rightshift-assign, found: %s",
		vm.type_name(fd.args[1]).c_str());
	return nullptr;
}

var_base_t *int_pow(vm_state_t &vm, const fn_data_t &fd)
{
	if(fd.args[1]->istype<var_int_t>()) {
		var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());
		mpz_pow_ui(res->get(), res->get(), mpz_get_ui(INT(fd.args[1])->get()));
		return res;
	} else if(fd.args[1]->istype<var_flt_t>()) {
		var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());
		mpz_t tmp;
		mpz_init(tmp);
		mpfr_get_z(tmp, FLT(fd.args[1])->get(), mpfr_get_default_rounding_mode());
		mpz_pow_ui(res->get(), res->get(), mpz_get_ui(tmp));
		mpz_clear(tmp);
		return res;
	}
	vm.fail(fd.src_id, fd.idx, "expected int or float argument for int power, found: %s",
		vm.type_name(fd.args[1]).c_str());
	return nullptr;
}

var_base_t *int_root(vm_state_t &vm, const fn_data_t &fd)
{
	if(fd.args[1]->istype<var_int_t>()) {
		var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());
		mpz_root(res->get(), res->get(), mpz_get_ui(INT(fd.args[1])->get()));
		return res;
	} else if(fd.args[1]->istype<var_flt_t>()) {
		var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());
		mpz_t tmp;
		mpz_init(tmp);
		mpfr_get_z(tmp, FLT(fd.args[1])->get(), mpfr_get_default_rounding_mode());
		mpz_root(res->get(), res->get(), mpz_get_ui(tmp));
		mpz_clear(tmp);
		return res;
	}
	vm.fail(fd.src_id, fd.idx, "expected int or float argument for int root, found: %s",
		vm.type_name(fd.args[1]).c_str());
	return nullptr;
}

var_base_t *int_preinc(vm_state_t &vm, const fn_data_t &fd)
{
	mpz_add_ui(INT(fd.args[0])->get(), INT(fd.args[0])->get(), 1);
	return fd.args[0];
}

var_base_t *int_postinc(vm_state_t &vm, const fn_data_t &fd)
{
	var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());
	mpz_add_ui(INT(fd.args[0])->get(), INT(fd.args[0])->get(), 1);
	return res;
}

var_base_t *int_predec(vm_state_t &vm, const fn_data_t &fd)
{
	mpz_sub_ui(INT(fd.args[0])->get(), INT(fd.args[0])->get(), 1);
	return fd.args[0];
}

var_base_t *int_postdec(vm_state_t &vm, const fn_data_t &fd)
{
	var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());
	mpz_sub_ui(INT(fd.args[0])->get(), INT(fd.args[0])->get(), 1);
	return res;
}

var_base_t *int_usub(vm_state_t &vm, const fn_data_t &fd)
{
	var_int_t *res = make<var_int_t>(INT(fd.args[0])->get());
	mpz_neg(res->get(), res->get());
	return res;
}

// logical functions

#endif // LIBRARY_CORE_INT_HPP