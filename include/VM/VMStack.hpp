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

#ifndef VM_VM_STACK_HPP
#define VM_VM_STACK_HPP

#include <vector>

#include "Vars/Base.hpp"

class vm_stack_t
{
	std::vector< var_base_t * > m_vec;
public:
	vm_stack_t();
	~vm_stack_t();

	void push( var_base_t * val, const bool iref = true );

	var_base_t * pop( const bool dref = true );

	inline var_base_t * & back() { return m_vec.back(); }

	inline std::vector< var_base_t * > & get() { return m_vec; }

	inline size_t size() const { return m_vec.size(); }

	inline bool empty() const { return m_vec.empty(); }

	vm_stack_t * thread_copy();
};

#endif // VM_VM_STACK_HPP
