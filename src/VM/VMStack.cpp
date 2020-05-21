/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
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
