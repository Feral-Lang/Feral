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

#include <string>
#include <unordered_map>

#include "Vars/Base.hpp"

/* vars for each source file */
class vars_t
{
	std::unordered_map< std::string, var_base_t * > m_vars;
public:
	vars_t();
	~vars_t();

	inline bool exists( const std::string & name ) { return m_vars.find( name ) != m_vars.end(); }
	var_base_t * get( const std::string & name );

	void add( const std::string & name, var_base_t * val );
	void rem( const std::string & name );
};

class var_stack_t
{
	std::vector< size_t > m_loops_from;
	// each vars_t is a stack frame
	std::vector< vars_t > m_stack;
public:
	var_stack_t();
	~var_stack_t();

	inline bool exists( const std::string & name ) { return m_stack.size() > 0 && m_stack.back().exists( name ); }
	var_base_t * get( const std::string & name );

	void add( const std::string & name, var_base_t * val );
	void rem( const std::string & name );
};

class var_src_t
{
	vars_t m_src_vars;
	// maps function id to vars_t
	std::unordered_map< size_t, var_stack_t > m_fn_vars;
public:
	var_src_t();
	~var_src_t();

	bool existsfn( const size_t & fn_id, const std::string & name );
	inline bool existssrc( const std::string & name ) { return m_src_vars.exists( name ); }

	var_base_t * getfn( const std::string & name );
	var_base_t * getsrc( const std::string & name );

	void addfn( const size_t & fn_id, const std::string & name, var_base_t * val );
	void addsrc( const std::string & name, var_base_t * val );

	void remfn( const size_t & fn_id, const std::string & name );
	void remsrc( const std::string & name );
};

#endif // VM_VARS_HPP
