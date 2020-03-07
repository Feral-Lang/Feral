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

	std::vector< std::string > inc_locs;
	std::vector< std::string > mod_locs;

	vm_state_t( const size_t & flags );
	~vm_state_t();

	void add_src( srcfile_t * src, const size_t & idx );
	void pop_src();

	void add_typefn( const size_t & type, const std::string & name, var_base_t * fn, const bool iref );
	var_fn_t * get_typefn( const size_t & type, const std::string & name );

	void gadd( const std::string & name, var_base_t * val, const bool iref = false );
	var_base_t * gget( const std::string & name );

	inline void add_in_fn( const bool in_fn ) { m_in_fn.push_back( in_fn ); }
	inline void rem_in_fn() { m_in_fn.pop_back(); }
	inline bool in_fn() const { return m_in_fn.size() > 0 ? m_in_fn.back() : false; }

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
	// functions for any and all types
	std::unordered_map< size_t, vars_frame_t * > m_typefns;
	// if execution is in a feral function or not
	std::vector< bool > m_in_fn;
};

typedef bool ( * mod_init_fn_t )( vm_state_t & vm );
#define REGISTER_MODULE( name )				\
	extern "C" bool init_##name( vm_state_t & vm )

template< typename T, typename ... Args > T * make( Args... args )
{
	T * res = new T( args... );
	res->dref();
	return res;
}

const char * nmod_ext();
const char * fmod_ext();

namespace vm
{

// fn_id = 0 = not in a function
int exec( vm_state_t & vm, const bcode_t & bcode, const size_t & fn_id = 0, const bool push_fn = true );

}

#endif // VM_VM_HPP
