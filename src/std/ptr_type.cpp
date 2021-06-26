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

var_ptr_t::var_ptr_t(var_base_t *val, const size_t &src_id, const size_t &idx)
	: var_base_t(type_id<var_ptr_t>(), src_id, idx, false, false), m_val(val)
{
	var_iref(m_val);
}
var_ptr_t::~var_ptr_t()
{
	var_dref(m_val);
}

var_base_t *var_ptr_t::copy(const size_t &src_id, const size_t &idx)
{
	return new var_ptr_t(m_val, src_id, idx);
}
void var_ptr_t::set(var_base_t *from)
{
	var_dref(m_val);
	m_val = PTR(from)->m_val;
	var_iref(m_val);
}

void var_ptr_t::update(var_base_t *with)
{
	var_dref(m_val);
	m_val = with;
	var_iref(m_val);
}

var_base_t *var_ptr_t::get()
{
	return m_val;
}