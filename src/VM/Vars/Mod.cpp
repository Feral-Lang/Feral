/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "../VM.hpp"

#include "Base.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////// SOME EXTRAS ///////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::unordered_map< std::string, var_base_t * > * var_mod_base()
{
	static std::unordered_map< std::string, var_base_t * > v;
	return & v;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////// VAR_MOD /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_module_t::var_module_t( srcfile_t * src, srcfile_vars_t * vars, const size_t & idx )
	: var_base_t( VT_MOD, idx, 1 ), m_src( src ), m_vars( vars ), m_copied( false )
{
	fuse( VT_MOD, var_mod_base() );
}
var_module_t::~var_module_t()
{
	if( !m_copied ) {
		if( m_vars ) delete m_vars;
		if( m_src ) delete m_src;
	}
}

var_base_t * var_module_t::copy( const size_t & idx )
{
	var_module_t * mod = new var_module_t( m_src, m_vars, idx );
	mod->m_copied = true;
	return mod;
}

srcfile_t * var_module_t::src() { return m_src; }
srcfile_vars_t * var_module_t::vars() { return m_vars; }
bool var_module_t::copied() { return m_copied; }

void var_module_t::set( var_base_t * from )
{
	var_module_t * f = MOD( from );
	if( !m_copied ) delete m_vars;
	m_src = f->m_src;
	m_vars = f->m_vars;
	f->m_copied = true;
}
