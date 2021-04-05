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

#include "VM/VMStack.hpp"

vm_stack_t::vm_stack_t() {}

vm_stack_t::~vm_stack_t()
{
	for( auto & val : m_vec ) {
		var_dref( val );
	}
}

void vm_stack_t::push( var_base_t * val, const bool iref )
{
	if( iref ) var_iref( val );
	m_vec.push_back( val );
}

var_base_t * vm_stack_t::pop( const bool dref )
{
	if( m_vec.size() == 0 ) return nullptr;
	var_base_t * back = nullptr;
	back = m_vec.back();
	m_vec.pop_back();
	if( dref ) var_dref( back );
	return back;
}


vm_stack_t * vm_stack_t::thread_copy()
{
	vm_stack_t * newstack = new vm_stack_t;
	for( auto & item : m_vec ) {
		newstack->m_vec.push_back( item->copy( item->src_id(), item->idx() ) );
	}
	return newstack;
}