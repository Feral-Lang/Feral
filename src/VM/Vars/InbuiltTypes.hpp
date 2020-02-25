/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef VM_VARS_INBUILT_TYPES_HPP
#define VM_VARS_INBUILT_TYPES_HPP

#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "Base.hpp"

class vartype_base_t : public var_base_t
{
	std::unordered_set< size_t > m_inherits;
protected:
	std::unordered_map< std::string, var_base_t * > m_attrs;
public:
	vartype_base_t( const size_t & type, const size_t & idx );
	vartype_base_t( const size_t & type, const std::unordered_map< std::string, var_base_t * > & attrs,
			const size_t & idx );
	virtual ~vartype_base_t();

	inline void inherit( const size_t & type ) { m_inherits.insert( type ); }
	inline bool inherits( const size_t & type ) const { return m_inherits.find( type ) != m_inherits.end(); }

	virtual void add_attr( const std::string & name, var_base_t * val );
	virtual var_base_t * get_attr( const std::string & name );
	bool attr_exists( const std::string & name );
};

// all built in types
class vartype_nil_t : public vartype_base_t
{
public:
	vartype_nil_t( const size_t & idx );
	~vartype_nil_t();
};

class vartype_bool_t : public vartype_base_t
{
public:
	vartype_bool_t( const size_t & idx );
	~vartype_bool_t();
};

class vartype_int_t : public vartype_base_t
{
public:
	vartype_int_t( const size_t & idx );
	~vartype_int_t();
};

class vartype_flt_t : public vartype_base_t
{
public:
	vartype_flt_t( const size_t & idx );
	~vartype_flt_t();
};

class vartype_str_t : public vartype_base_t
{
public:
	vartype_str_t( const size_t & idx );
	~vartype_str_t();
};

class vartype_fn_t : public vartype_base_t
{
public:
	vartype_fn_t( const size_t & idx );
	~vartype_fn_t();
};

class vartype_type_t : public vartype_base_t
{
	std::vector< std::string > m_fields_pos;
public:
	vartype_type_t( const std::vector< std::string > & fields_pos,
			std::unordered_map< std::string, var_base_t * > & attrs,
			const size_t & idx );
	~vartype_type_t();

	// specialized virtual function
	void add_attr( const std::string & name, var_base_t * val );
	size_t get_attr_pos( const std::string & name );
};

#endif // VM_VARS_INBUILT_TYPES_HPP
