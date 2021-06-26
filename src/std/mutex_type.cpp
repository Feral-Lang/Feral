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

#include "std/mutex_type.hpp"

var_mutex_t::var_mutex_t(const size_t &src_id, const size_t &idx)
	: var_base_t(type_id<var_mutex_t>(), src_id, idx, false, false)
{}

var_base_t *var_mutex_t::copy(const size_t &src_id, const size_t &idx)
{
	return new var_mutex_t(src_id, idx);
}
void var_mutex_t::set(var_base_t *from)
{
	// nothing to reassign in a mutex
}