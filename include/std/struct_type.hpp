/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef STRUCT_HPP
#define STRUCT_HPP

#include "../../src/VM/VM.hpp"

class var_struct_def_t : public var_base_t
{
	std::vector< std::string > m_attr_order;
	std::unordered_map< std::string, var_base_t * > m_attrs;
	// type id of struct which will be used as m_type for struct objects
	std::uintptr_t m_id;

public:
	var_struct_def_t( const std::uintptr_t & id, const std::vector< std::string > & attr_order,
			  const std::unordered_map< std::string, var_base_t * > & attrs,
			  const size_t & src_id, const size_t & idx );
	~var_struct_def_t();

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	// returns var_struct_t
	var_base_t * call( vm_state_t & vm, const std::vector< var_base_t * > & args,
			   const std::vector< fn_assn_arg_t > & assn_args,
			   const std::unordered_map< std::string, size_t > & assn_args_loc,
			   const size_t & src_id, const size_t & idx );

	bool attr_exists( const std::string & name ) const;
	void attr_set( const std::string & name, var_base_t * val, const bool iref );
	var_base_t * attr_get( const std::string & name );

	const std::vector< std::string > & attr_order() const;
	const std::unordered_map< std::string, var_base_t * > & attrs() const;
	std::uintptr_t typefn_id() const;
};
#define STRUCT_DEF( x ) static_cast< var_struct_def_t * >( x )

class var_struct_t : public var_base_t
{
	std::unordered_map< std::string, var_base_t * > m_attrs;
	std::uintptr_t m_id;
	var_struct_def_t * m_base;
public:
	var_struct_t( const std::uintptr_t & struct_id, const std::unordered_map< std::string, var_base_t * > & attrs,
		      var_struct_def_t * base, const size_t & src_id, const size_t & idx );
	~var_struct_t();

	std::uintptr_t typefn_id() const;

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	bool attr_exists( const std::string & name ) const;
	void attr_set( const std::string & name, var_base_t * val, const bool iref );
	var_base_t * attr_get( const std::string & name );

	const std::vector< std::string > & attr_order() const;
	const std::unordered_map< std::string, var_base_t * > & attrs() const;
	var_struct_def_t * base() const;
};
#define STRUCT( x ) static_cast< var_struct_t * >( x )

#endif // STRUCT_HPP