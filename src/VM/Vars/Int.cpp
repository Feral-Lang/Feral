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

#include "VM/Vars/Base.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VAR_INT //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

var_int_t::var_int_t(const mpz_t val, const size_t &src_id, const size_t &idx)
	: var_base_t(type_id<var_int_t>(), src_id, idx, false, false)
{
	mpz_init_set(m_val, val);
}
var_int_t::var_int_t(const int &val, const size_t &src_id, const size_t &idx)
	: var_base_t(type_id<var_int_t>(), src_id, idx, false, false)
{
	mpz_init_set_si(m_val, val);
}
var_int_t::var_int_t(const mpfr_t val, const size_t &src_id, const size_t &idx)
	: var_base_t(type_id<var_int_t>(), src_id, idx, false, false)
{
	mpz_init(m_val);
	mpfr_get_z(m_val, val, mpfr_get_default_rounding_mode());
}
var_int_t::var_int_t(const char *val, const size_t &src_id, const size_t &idx)
	: var_base_t(type_id<var_int_t>(), src_id, idx, false, false)
{
	mpz_init_set_str(m_val, val, 0);
}
var_int_t::~var_int_t()
{
	mpz_clear(m_val);
}

var_base_t *var_int_t::copy(const size_t &src_id, const size_t &idx)
{
	return new var_int_t(m_val, src_id, idx);
}
mpz_t &var_int_t::get()
{
	return m_val;
}
void var_int_t::set(var_base_t *from)
{
	mpz_set(m_val, INT(from)->get());
}