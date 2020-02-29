/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef VM_VARS_HPP
#define VM_VARS_HPP

#include <vector>
#include <string>
#include <unordered_map>

#include "Vars/Base.hpp"

class vars_t
{
	std::unordered_map< std::string, var_base_t * > m_vars;
public:
	vars_t();
	~vars_t();

	inline bool exists( const std::string & name ) { return m_vars.find( name ) != m_vars.end(); }
	var_base_t * get( const std::string & name );

	void add( const std::string & name, var_base_t * val, const bool inc_ref );
	void rem( const std::string & name, const bool dec_ref );

	static void * operator new( size_t sz );
	static void operator delete( void * ptr, size_t sz );
};

class var_stack_t
{
	std::vector< size_t > m_loops_from;
	// each vars_t is a stack frame
	// it is a pointer to remove the issue of recreation of object when vector increases size
	// since recreation will cause the object to be deleted (and destructor be called) and invalidate
	// the variable pointers (since destructor contains var_dref() calls)
	std::vector< vars_t * > m_stack;
	size_t m_size;
public:
	var_stack_t();
	~var_stack_t();

	bool exists( const std::string & name, const bool all_scopes );
	var_base_t * get( const std::string & name );

	void inc_top( const size_t & count );
	void dec_top( const size_t & count );

	void add( const std::string & name, var_base_t * val, const bool inc_ref );
	void rem( const std::string & name, const bool dec_ref );
};

/* vars for each source file */
class srcfile_vars_t
{
	var_stack_t m_src_vars;
	std::vector< size_t > m_curr_fn_stack;
	// maps function id to vars_t
	std::unordered_map< size_t, var_stack_t * > m_fn_vars;
public:
	srcfile_vars_t();
	~srcfile_vars_t();

	bool exists( const std::string & name, const bool in_fn, const bool all_scopes );

	var_base_t * get( const std::string & name );

	void blk_add( const size_t & count, const bool in_fn );
	void blk_rem( const size_t & count, const bool in_fn );

	void push_fn_id( const size_t & id );
	void pop_fn_id();

	void add( const std::string & name, var_base_t * val, const bool in_fn, const bool inc_ref );
	void rem( const std::string & name, const bool in_fn, const bool dec_ref );
};

#endif // VM_VARS_HPP
