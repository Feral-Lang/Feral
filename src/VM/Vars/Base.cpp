/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Base.hpp"

static size_t type_id()
{
	static size_t tid = 0;
	return tid++;
}

var_base_t::var_base_t( const size_t & idx, const size_t & ref )
	: m_idx( idx ), m_ref( ref ) {}
var_base_t::~var_base_t() {}

vartype_base_t::vartype_base_t( const std::vector< std::string > & fields_pos,
				const std::unordered_map< std::string, var_base_t * > & fields )
	: m_id( type_id() ), m_fields_pos( fields_pos ), m_fields( fields ) {}
vartype_base_t::~vartype_base_t() {}
