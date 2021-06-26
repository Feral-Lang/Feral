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

#ifndef LIBRARY_CORE_FLT_HPP
#define LIBRARY_CORE_FLT_HPP

#include "VM/VM.hpp"

#define ARITHF_FUNC(name)                                                                        \
	var_base_t *flt_##name(vm_state_t &vm, const fn_data_t &fd)                              \
	{                                                                                        \
		if(fd.args[1]->istype<var_int_t>()) {                                            \
			var_flt_t *res = make<var_flt_t>(FLT(fd.args[0])->get());                \
			mpfr_t tmp;                                                              \
			mpfr_init_set_z(tmp, INT(fd.args[1])->get(),                             \
					mpfr_get_default_rounding_mode());                       \
			mpfr_##name(res->get(), res->get(), tmp,                                 \
				    mpfr_get_default_rounding_mode());                           \
			mpfr_clear(tmp);                                                         \
			return res;                                                              \
		} else if(fd.args[1]->istype<var_flt_t>()) {                                     \
			var_flt_t *res = make<var_flt_t>(FLT(fd.args[0])->get());                \
			mpfr_##name(res->get(), res->get(), FLT(fd.args[1])->get(),              \
				    mpfr_get_default_rounding_mode());                           \
			return res;                                                              \
		}                                                                                \
		vm.fail(fd.src_id, fd.idx,                                                       \
			"expected int or float argument for flt " STRINGIFY(name) ", found: %s", \
			vm.type_name(fd.args[1]).c_str());                                       \
		return nullptr;                                                                  \
	}

#define ARITHF_ASSN_FUNC(name)                                                                  \
	var_base_t *flt_assn_##name(vm_state_t &vm, const fn_data_t &fd)                        \
	{                                                                                       \
		if(fd.args[1]->istype<var_int_t>()) {                                           \
			mpfr_t tmp;                                                             \
			mpfr_init_set_z(tmp, INT(fd.args[1])->get(),                            \
					mpfr_get_default_rounding_mode());                      \
			mpfr_##name(FLT(fd.args[0])->get(), FLT(fd.args[0])->get(), tmp,        \
				    mpfr_get_default_rounding_mode());                          \
			mpfr_clear(tmp);                                                        \
			return fd.args[0];                                                      \
		} else if(fd.args[1]->istype<var_flt_t>()) {                                    \
			mpfr_##name(FLT(fd.args[0])->get(), FLT(fd.args[0])->get(),             \
				    FLT(fd.args[1])->get(), mpfr_get_default_rounding_mode());  \
			return fd.args[0];                                                      \
		}                                                                               \
		vm.fail(                                                                        \
		fd.src_id, fd.idx,                                                              \
		"expected int or float argument for flt " STRINGIFY(name) "-assign, found: %s", \
		vm.type_name(fd.args[1]).c_str());                                              \
		return nullptr;                                                                 \
	}

#define LOGICF_FUNC(name, sym)                                                                \
	var_base_t *flt_##name(vm_state_t &vm, const fn_data_t &fd)                           \
	{                                                                                     \
		if(fd.args[1]->istype<var_flt_t>()) {                                         \
			return mpfr_cmp(FLT(fd.args[0])->get(), FLT(fd.args[1])->get()) sym 0 \
			       ? vm.tru                                                       \
			       : vm.fals;                                                     \
		}                                                                             \
		vm.fail(fd.src_id, fd.idx,                                                    \
			"expected flt argument for flt " STRINGIFY(name) ", found: %s",       \
			vm.type_name(fd.args[1]).c_str());                                    \
		return nullptr;                                                               \
	}

ARITHF_FUNC(add)
ARITHF_FUNC(sub)
ARITHF_FUNC(mul)
ARITHF_FUNC(div)

ARITHF_ASSN_FUNC(add)
ARITHF_ASSN_FUNC(sub)
ARITHF_ASSN_FUNC(mul)
ARITHF_ASSN_FUNC(div)

LOGICF_FUNC(lt, <)
LOGICF_FUNC(gt, >)
LOGICF_FUNC(le, <=)
LOGICF_FUNC(ge, >=)

var_base_t *flt_eq(vm_state_t &vm, const fn_data_t &fd)
{
	if(fd.args[1]->istype<var_flt_t>()) {
		return mpfr_cmp(FLT(fd.args[0])->get(), FLT(fd.args[1])->get()) == 0 ? vm.tru
										     : vm.fals;
	}
	return vm.fals;
}

var_base_t *flt_ne(vm_state_t &vm, const fn_data_t &fd)
{
	if(fd.args[1]->istype<var_flt_t>()) {
		return mpfr_cmp(FLT(fd.args[0])->get(), FLT(fd.args[1])->get()) != 0 ? vm.tru
										     : vm.fals;
	}
	return vm.tru;
}

var_base_t *flt_preinc(vm_state_t &vm, const fn_data_t &fd)
{
	mpfr_add_ui(FLT(fd.args[0])->get(), FLT(fd.args[0])->get(), 1,
		    mpfr_get_default_rounding_mode());
	return fd.args[0];
}

var_base_t *flt_postinc(vm_state_t &vm, const fn_data_t &fd)
{
	var_flt_t *res = make<var_flt_t>(FLT(fd.args[0])->get());
	mpfr_add_ui(FLT(fd.args[0])->get(), FLT(fd.args[0])->get(), 1,
		    mpfr_get_default_rounding_mode());
	return res;
}

var_base_t *flt_predec(vm_state_t &vm, const fn_data_t &fd)
{
	mpfr_sub_ui(FLT(fd.args[0])->get(), FLT(fd.args[0])->get(), 1,
		    mpfr_get_default_rounding_mode());
	return fd.args[0];
}

var_base_t *flt_postdec(vm_state_t &vm, const fn_data_t &fd)
{
	var_flt_t *res = make<var_flt_t>(FLT(fd.args[0])->get());
	mpfr_sub_ui(FLT(fd.args[0])->get(), FLT(fd.args[0])->get(), 1,
		    mpfr_get_default_rounding_mode());
	return res;
}

var_base_t *flt_usub(vm_state_t &vm, const fn_data_t &fd)
{
	var_flt_t *res = make<var_flt_t>(FLT(fd.args[0])->get());
	mpfr_neg(res->get(), FLT(fd.args[0])->get(), mpfr_get_default_rounding_mode());
	return res;
}

var_base_t *flt_round(vm_state_t &vm, const fn_data_t &fd)
{
	var_int_t *res = make<var_int_t>(0);
	mpfr_get_z(res->get(), FLT(fd.args[0])->get(), MPFR_RNDN);
	return res;
}

var_base_t *flt_pow(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_int_t>()) {
		vm.fail(fd.src_id, fd.idx, "power must be an integer, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}
	var_flt_t *res = make<var_flt_t>(FLT(fd.args[0])->get());
	mpfr_pow_si(res->get(), FLT(fd.args[0])->get(), mpz_get_si(INT(fd.args[1])->get()),
		    MPFR_RNDN);
	return res;
}

var_base_t *flt_root(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_int_t>()) {
		vm.fail(fd.src_id, fd.idx, "root must be an integer, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}
	var_flt_t *res = make<var_flt_t>(FLT(fd.args[0])->get());
#if MPFR_VERSION_MAJOR >= 4
	mpfr_rootn_ui(res->get(), FLT(fd.args[0])->get(), mpz_get_ui(INT(fd.args[1])->get()),
		      MPFR_RNDN);
#else
	mpfr_root(res->get(), FLT(fd.args[0])->get(), mpz_get_ui(INT(fd.args[1])->get()),
		  MPFR_RNDN);
#endif // MPFR_VERSION_MAJOR
	return res;
}

#endif // LIBRARY_CORE_FLT_HPP