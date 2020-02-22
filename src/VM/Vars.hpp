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

#include "Vars/InbuiltTypes.hpp"

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
};

class var_stack_t
{
	std::vector< size_t > m_loops_from;
	// each vars_t is a stack frame
	std::vector< vars_t > m_stack;
	size_t m_top;
public:
	var_stack_t();
	~var_stack_t();

	bool exists( const std::string & name, const bool all_scopes );
	var_base_t * get( const std::string & name );

	void inc_top( const size_t & count );
	void dec_top( const size_t & count );

	void add( const std::string & name, var_base_t * val, const bool inc_ref );
	// adds variables to next value of top
	void add_no_inc();
	void rem( const std::string & name, const bool dec_ref );
};

/* vars for each source file */
class var_srcfile_t
{
	vars_t m_src_vars;
	std::vector< size_t > m_curr_fn_stack;
	// maps function id to vars_t
	std::unordered_map< size_t, var_stack_t > m_fn_vars;
public:
	var_srcfile_t();
	~var_srcfile_t();

	bool exists( const std::string & name, const bool in_fn, const bool all_scopes );

	var_base_t * get( const std::string & name );

	inline void blk_add( const size_t & count ) { m_fn_vars[ m_curr_fn_stack.back() ].inc_top( count ); }
	inline void blk_rem( const size_t & count ) { m_fn_vars[ m_curr_fn_stack.back() ].dec_top( count ); }

	inline void push_fn_id( const size_t & id ) { m_curr_fn_stack.push_back( id ); }
	inline void pop_fn_id() { if( m_curr_fn_stack.size() > 0 ) m_curr_fn_stack.pop_back(); }

	void add( const std::string & name, var_base_t * val, const bool in_fn, const bool inc_ref );
	void rem( const std::string & name, const bool in_fn, const bool dec_ref );
};

#endif // VM_VARS_HPP
