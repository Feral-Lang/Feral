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

#include "VM/VMFailStack.hpp"

vm_failstack_t::vm_failstack_t() {}

vm_failstack_t::~vm_failstack_t()
{
	assert( m_stack.size() == 0 );
}

void vm_failstack_t::push( var_base_t * val, const bool iref )
{
	if( iref ) var_iref( val );
	m_stack.back().push_back( val );
}

var_base_t * vm_failstack_t::pop( const bool dref )
{
	if( m_stack.size() == 0 || m_stack.back().size() == 0 ) return nullptr;
	var_base_t * front = nullptr;
	front = m_stack.back().front();
	m_stack.back().pop_front();
	if( dref ) var_dref( front );
	return front;
}
