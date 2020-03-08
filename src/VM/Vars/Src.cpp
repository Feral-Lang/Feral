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
////////////////////////////////////////////////////////////// VAR_MOD /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_src_t::var_src_t( srcfile_t * src, vars_t * vars, const size_t & src_id, const size_t & idx )
	: var_attr_based_t( VT_SRC, src_id, idx ), m_src( src ), m_vars( vars ), m_copied( false )
{
}
var_src_t::~var_src_t()
{
	if( !m_copied ) {
		if( m_vars ) delete m_vars;
		if( m_src ) delete m_src;
	}
}

var_base_t * var_src_t::copy( const size_t & src_id, const size_t & idx )
{
	var_src_t * mod = new var_src_t( m_src, m_vars, src_id, idx );
	mod->m_copied = true;
	return mod;
}

void var_src_t::set( var_base_t * from )
{
	var_src_t * f = SRC( from );
	if( !m_copied ) delete m_vars;
	m_src = f->m_src;
	m_vars = f->m_vars;
	f->m_copied = true;
}

bool var_src_t::attr_exists( const std::string & name ) const
{
	return m_vars->exists( name );
}

void var_src_t::attr_set( const std::string & name, var_base_t * val, const bool iref )
{
	m_vars->add( name, val, iref );
}

var_base_t * var_src_t::attr_get( const std::string & name )
{
	return m_vars->get( name );
}

void var_src_t::add_nativefn( const std::string & name, nativefnptr_t body,
			      const std::vector< std::string > & args,
			      const bool is_va )
{
	m_vars->add( name, new var_fn_t( m_src->path(), "", is_va ? "." : "", args, {}, { .native = body }, true, m_src->id(), 0 ), false );
}

srcfile_t * var_src_t::src() { return m_src; }
vars_t * var_src_t::vars() { return m_vars; }
bool var_src_t::copied() { return m_copied; }
