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
////////////////////////////////////////// VAR_TYPEID ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

var_typeid_t::var_typeid_t(const std::uintptr_t &val, const size_t &src_id, const size_t &idx)
	: var_base_t(type_id<var_typeid_t>(), src_id, idx, false, false), m_val(val)
{}

var_base_t *var_typeid_t::copy(const size_t &src_id, const size_t &idx)
{
	return new var_typeid_t(m_val, src_id, idx);
}
void var_typeid_t::set(var_base_t *from) { m_val = TYPEID(from)->get(); }
std::uintptr_t &var_typeid_t::get() { return m_val; }