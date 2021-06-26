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

#ifndef VEC_TYPE_HPP
#define VEC_TYPE_HPP

#include "../VM/VM.hpp"

class var_vec_iterable_t : public var_base_t
{
	var_vec_t *m_vec;
	size_t m_curr;

public:
	var_vec_iterable_t(var_vec_t *vec, const size_t &src_id, const size_t &idx);
	~var_vec_iterable_t();

	var_base_t *copy(const size_t &src_id, const size_t &idx);
	void set(var_base_t *from);

	bool next(var_base_t *&val);
};
#define VEC_ITERABLE(x) static_cast<var_vec_iterable_t *>(x)

#endif // VEC_TYPE_HPP