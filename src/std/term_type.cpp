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

#include "std/term_type.hpp"

var_term_t::var_term_t(const struct termios &term, const size_t &src_id, const size_t &idx)
	: var_base_t(type_id<var_term_t>(), src_id, idx, false, false), m_term(term)
{}

var_base_t *var_term_t::copy(const size_t &src_id, const size_t &idx)
{
	return new var_term_t(m_term, src_id, idx);
}

void var_term_t::set(var_base_t *from)
{
	m_term = TERM(from)->get();
}