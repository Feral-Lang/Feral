/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef VM_VM_HPP
#define VM_VM_HPP

#include <string>
#include <unordered_map>

#include "DyLib.hpp"
#include "SrcFile.hpp"
#include "VMStack.hpp"

typedef std::vector< srcfile_t * > src_stack_t;

// maps source path with its ID
typedef std::unordered_map< std::string, size_t > all_srcs_t;

struct vm_state_t
{
	size_t exit_code;
	size_t exec_flags;
	src_stack_t src_stack;
	all_srcs_t all_srcs;
	vm_stack_t * vm_stack;

	// globally common variables
	var_base_t * tru;
	var_base_t * fals;
	var_base_t * nil;

	// this is a pointer since it must be explicitly deleted after everything else
	dyn_lib_t * dlib;

	vm_state_t( const size_t & flags );
	~vm_state_t();

	void add_src( srcfile_t * src );
	void pop_src();

	inline void register_vartype( vartype_base_t * type ) { m_vartypes[ type->type() ] = type; }
	vartype_base_t * get_vartype( const size_t & type_id );

private:
	// all global variable types
	std::unordered_map< size_t, vartype_base_t * > m_vartypes;
};

struct func_call_data_t
{
};

namespace vm
{

inline srcfile_t src_new( const std::string & dir, const std::string & path,
			  const bool is_main_src = false )
{
	static size_t id = 0;
	auto src = srcfile_t( id++, dir, path, is_main_src );
	return src;
}

// fn_id = 0 = not in a function
int exec( vm_state_t & vm, const size_t & fn_id = 0 );

}

#endif // VM_VM_HPP
