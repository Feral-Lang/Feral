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

#ifndef VM_VM_FAIL_STACK_HPP
#define VM_VM_FAIL_STACK_HPP

#include <vector>
#include <deque>

#include "Vars/Base.hpp"

class vm_failstack_t
{
	std::vector< std::deque< var_base_t * > > m_stack;
public:
	vm_failstack_t();
	~vm_failstack_t();

	inline void blka() { m_stack.push_back( std::deque< var_base_t * >{} ); }
	inline void blkr() { for( auto & e : m_stack.back() ) var_dref( e ); m_stack.pop_back(); }

	void push( var_base_t * val, const bool iref = true );
	var_base_t * pop( const bool dref = true );

	inline size_t size() const { return m_stack.size(); }
	inline bool empty() const { return m_stack.empty(); }
	inline bool backempty() const { return m_stack.back().empty(); }
};

#endif // VM_VM_FAIL_STACK_HPP
