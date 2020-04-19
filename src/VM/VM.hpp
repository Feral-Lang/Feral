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

typedef bool ( * mod_init_fn_t )( vm_state_t & vm, const size_t src_id, const size_t & idx );
typedef void ( * mod_deinit_fn_t )();
#define INIT_MODULE( name )			\
	extern "C" bool init_##name( vm_state_t & vm, const size_t src_id, const size_t & idx )
#define DEINIT_MODULE( name )			\
	extern "C" void deinit_##name()

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

	// this is a pointer since it must be explicitly deleted after everything else
	dyn_lib_t * dlib;

	// arguments for feral source from command line
	var_base_t * src_args;

	vm_state_t( const std::string & self_binary_loc, const std::vector< std::string > & args, const size_t & flags );
	~vm_state_t();

	void push_src( srcfile_t * src, const size_t & idx );
	void push_src( const std::string & src_path );
	void pop_src();

	// modules & imports
	// nmod = native module
	// fmod = feral module
	bool mod_exists( const std::vector< std::string > & locs, std::string & mod, const std::string & ext );
	bool load_nmod( const std::string & mod_str, const size_t & src_id, const size_t & idx,
			const bool set_dll_core_load_loc = false );
	int load_fmod( const std::string & mod_file );

	inline void set_fmod_load_fn( fmod_load_fn_t load_fn ) { m_src_load_fn = load_fn; }

	inline const std::vector< std::string > & inc_locs() const { return m_inc_locs; }
	inline const std::vector< std::string > & dll_locs() const { return m_dll_locs; }

	inline var_src_t * current_source() const { return src_stack.back(); }
	inline srcfile_t * current_source_file() const { return src_stack.back()->src(); }

	void gadd( const std::string & name, var_base_t * val, const bool iref = true );
	var_base_t * gget( const std::string & name );

	int register_struct_enum_id();

	int register_new_type( const std::string & name, const size_t & src_id, const size_t & idx );
	// returns 0 on failure because no dll type can have id >= 0
	// see vm_state_t() -> m_custom_types for more info
	int dll_typeid( const std::string & name );

	void add_typefn( const int & type, const std::string & name, var_base_t * fn, const bool iref );
	inline void add_native_typefn( const int & type, const std::string & name, nativefnptr_t fn,
				       const size_t & args_count, const size_t & src_id, const size_t & idx )
	{
		add_typefn( type, name, new var_fn_t( src_stack.back()->src()->path(),
						      std::vector< std::string >( args_count, "" ),
						      {}, { .native = fn }, src_id, idx ), false );
	}
	var_fn_t * get_typefn( const int & type, const std::string & name );

	// used to convert typeid -> name
	void set_typename( const int & type, const std::string & name );
	std::string type_name( const int & type );

	inline const std::string & self_binary() const { return m_self_binary; }
	inline const std::string & dll_core_load_loc() const { return m_dll_core_load_loc; }
	inline const std::string & feral_home_dir() { return m_feral_home_dir; }

	void fail( const size_t & idx, const char * msg, ... ) const;

	bool load_core_mods();
private:
	// file loading function
	fmod_load_fn_t m_src_load_fn;
	// include and module locations - searches in increasing order of vector elements
	std::vector< std::string > m_inc_locs;
	std::vector< std::string > m_dll_locs;
	// global vars/objects that are required
	std::unordered_map< std::string, var_base_t * > m_globals;
	// type ids for custom types (negative)
	int m_custom_types;
	// functions for any and all types
	std::unordered_map< int, vars_frame_t * > m_typefns;
	// names of types (optional)
	std::unordered_map< int, std::string > m_typenames;
	// location where feral binary exists (used by sys.self_binary())
	std::string m_self_binary;
	// directory where (core) module is loaded from (used by builder)
	std::string m_dll_core_load_loc;
	// directory where feral libraries and config and stuff lives
	std::string m_feral_home_dir;
	// dll type name to id mapping
	std::unordered_map< std::string, int > m_dll_typenames;
	// all functions to call before unloading dlls
	std::unordered_map< std::string, mod_deinit_fn_t > m_dll_deinit_fns;
};

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
const char * fmod_ext( const bool compiled = false );

namespace vm
{

// end = 0 = till size of bcode
int exec( vm_state_t & vm, const size_t & begin = 0, const size_t & end = 0 );

}

#endif // VM_VM_HPP
