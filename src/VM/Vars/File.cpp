/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Base.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// VAR_FILE /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_file_t::var_file_t( FILE * const file, const std::string & mode, const size_t & src_id, const size_t & idx, const bool owner )
	: var_base_t( VT_FILE, src_id, idx, false, false ), m_file( file ), m_mode( mode ), m_owner( owner )
{}
var_file_t::~var_file_t()
{
	if( m_owner ) fclose( m_file );
}

void * var_file_t::get_data( const size_t & idx )
{
	if( idx == 0 ) return m_file;
	else if( idx == 1 ) return & m_mode;
	return nullptr;
}

var_base_t * var_file_t::copy( const size_t & src_id, const size_t & idx )
{
	return new var_file_t( m_file, m_mode, src_id, idx, false );
}

void var_file_t::set( var_base_t * from )
{
	if( m_owner ) fclose( m_file );
	m_owner = false;
	m_file = FILE( from )->get();
}
