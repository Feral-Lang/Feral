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
#include "Vars.hpp"
#include "VMStack.hpp"

typedef std::vector< var_src_t * > src_stack_t;

typedef std::unordered_map< std::string, var_src_t * > all_srcs_t;

typedef srcfile_t * ( * fmod_load_fn_t )( const std::string & src_file, const size_t & flags, const bool is_main_src, Errors & err );

struct vm_state_t
{
	bool exit_called;
	size_t exit_code;
	size_t exec_flags;

	src_stack_t src_stack;
	all_srcs_t all_srcs;
	vm_stack_t * vm_stack;

	// globally common variables
	var_base_t * tru;
	var_base_t * fals;
	var_base_t * nil;
	var_base_t * args;

	// location where feral binary exists (used by sys.self_binary())
	std::string self_binary;

	// this is a pointer since it must be explicitly deleted after everything else
	dyn_lib_t * dlib;

	std::vector< std::string > inc_locs;
	std::vector< std::string > mod_locs;

	var_base_t * src_args;

	vm_state_t( const std::string & self_binary_loc, const std::vector< std::string > & args, const size_t & flags );
	~vm_state_t();

	void push_src( srcfile_t * src, const size_t & idx );
	void push_src( const std::string & src_path );
	void pop_src();

	int register_new_type();

	void add_typefn( const int & type, const std::string & name, var_base_t * fn, const bool iref );
	var_fn_t * get_typefn( const int & type, const std::string & name );

	void set_typename( const int & type, const std::string & name );
	std::string type_name( const int & type );

	void gadd( const std::string & name, var_base_t * val, const bool iref = false );
	var_base_t * gget( const std::string & name );

	// modules & imports
	// nmod = native module
	// fmod = feral module
	bool mod_exists( const std::vector< std::string > & locs, std::string & mod, const std::string & ext );
	bool load_nmod( const std::string & mod_str, const size_t & idx );
	int load_fmod( const std::string & mod_file );

	inline void set_fmod_load_fn( fmod_load_fn_t load_fn ) { m_src_load_fn = load_fn; }

	bool load_core_mods();
private:
	// file loading function
	fmod_load_fn_t m_src_load_fn;
	// global vars/objects that are required
	std::unordered_map< std::string, var_base_t * > m_globals;
	// type ids for custom types (negative)
	int m_custom_types;
	// functions for any and all types
	std::unordered_map< int, vars_frame_t * > m_typefns;
	// names of types (optional)
	std::unordered_map< int, std::string > m_typenames;
};

typedef bool ( * mod_init_fn_t )( vm_state_t & vm );
#define REGISTER_MODULE( name )				\
	extern "C" bool init_##name( vm_state_t & vm )

template< typename T, typename ... Args > T * make( Args... args )
{
	// 0, 0 for src_id and idx
	T * res = new T( args..., 0, 0 );
	res->dref();
	return res;
}

template< typename T, typename ... Args > T * make_all( Args... args )
{
	// 0, 0 for src_id and idx
	T * res = new T( args... );
	res->dref();
	return res;
}

const char * nmod_ext();
const char * fmod_ext();

namespace vm
{

// end = 0 = till size of bcode
int exec( vm_state_t & vm, const size_t & begin = 0, const size_t & end = 0 );

}

#endif // VM_VM_HPP
