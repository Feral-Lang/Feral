/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef VM_VAR_TYPES_BASE_HPP
#define VM_VAR_TYPES_BASE_HPP

#include <vector>
#include <string>
#include <unordered_map>

class var_base_t
{
	size_t m_idx;
	size_t m_ref;
public:
	var_base_t( const size_t & idx, const size_t & ref );
	virtual ~var_base_t();

	inline void idx( const size_t & new_idx ) { m_idx = new_idx; }
	inline size_t idx() const { return m_idx; }

	inline void iref() { ++m_ref; }
	inline size_t dref() { if( m_ref >= 1 ) --m_ref; return m_ref; }
	inline size_t ref() const { return m_ref; }
};

class vartype_base_t
{
	size_t m_id;
	std::vector< std::string > m_fields_pos;
	std::unordered_map< std::string, var_base_t * > m_fields;
public:
	vartype_base_t( const std::vector< std::string > & fields_pos,
			const std::unordered_map< std::string, var_base_t * > & fields );
	virtual ~vartype_base_t();
};

#endif // VM_VAR_TYPES_BASE_HPP
