/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
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

	void push_back( var_base_t * val, const bool iref = true );

	var_base_t * pop_back( const bool dref = true );

	inline var_base_t * & back() { return m_vec.back(); }

	inline std::vector< var_base_t * > & get() { return m_vec; }

	inline size_t size() const { return m_vec.size(); }

	inline bool empty() const { return m_vec.empty(); }
};

#endif // VM_VM_STACK_HPP
