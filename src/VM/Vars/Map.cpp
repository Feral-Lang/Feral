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
/////////////////////////////////////////// VAR_MAP //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

var_map_t::var_map_t(const std::map<std::string, var_base_t *> &val, const bool &refs,
		     const size_t &src_id, const size_t &idx)
	: var_base_t(type_id<var_map_t>(), src_id, idx, false, false), m_val(val), m_refs(refs)
{}
var_map_t::~var_map_t()
{
	for(auto &v : m_val) var_dref(v.second);
}

var_base_t *var_map_t::copy(const size_t &src_id, const size_t &idx)
{
	std::map<std::string, var_base_t *> new_map;
	for(auto &v : m_val) {
		new_map[v.first] = v.second->copy(src_id, idx);
	}
	return new var_map_t(new_map, m_refs, src_id, idx);
}

void var_map_t::set(var_base_t *from)
{
	for(auto &v : m_val) {
		var_dref(v.second);
	}
	m_val.clear();
	for(auto &v : MAP(from)->m_val) {
		var_iref(v.second);
	}
	m_val  = MAP(from)->m_val;
	m_refs = MAP(from)->m_refs;
}

std::map<std::string, var_base_t *> &var_map_t::get()
{
	return m_val;
}
bool var_map_t::is_ref_map()
{
	return m_refs;
}