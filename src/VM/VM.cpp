/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <string>
#include <cstdlib>

#include "VM.hpp"

vm_state_t::vm_state_t( const size_t & flags )
	: exit_code( 0 ), exec_flags( flags ), tru( new var_bool_t( true, 0 ) ),
	  fals( new var_bool_t( false, 0 ) ), nil( new var_nil_t( 0 ) ),
	  vm_stack( new vm_stack_t() ), dlib( new dyn_lib_t() )
{
	init_builtin_types( * this );
}

vm_state_t::~vm_state_t()
{
	for( auto & st : m_structs ) var_dref( st.second );
	for( auto & src : src_stack ) delete src;
	delete vm_stack;
	var_dref( nil );
	var_dref( fals );
	var_dref( tru );
	delete dlib;
}

void vm_state_t::add_src( srcfile_t * src )
{
	if( all_srcs.find( src->get_path() ) != all_srcs.end() ) {
		all_srcs[ src->get_path() ] = src->get_id();
	}
	src_stack.push_back( src );
}

void vm_state_t::pop_src() { if( src_stack.size() > 0 ) src_stack.pop_back(); }

var_struct_t * vm_state_t::get_struct( const size_t & type_id )
{
	if( m_structs.find( type_id ) == m_structs.end() ) return nullptr;
	return m_structs[ type_id ];
}
