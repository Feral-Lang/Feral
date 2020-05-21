/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "VM/VM.hpp"
#include "VM/Vars/Base.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////// VAR_MOD /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_src_t::var_src_t( srcfile_t * src, vars_t * vars, const size_t & src_id, const size_t & idx, const bool owner )
	: var_base_t( type_id< var_src_t >(), src_id, idx, false, true ), m_src( src ), m_vars( vars ), m_owner( owner )
{}
var_src_t::~var_src_t()
{
	if( m_owner ) {
		if( m_vars ) delete m_vars;
		if( m_src ) delete m_src;
	}
}

var_base_t * var_src_t::copy( const size_t & src_id, const size_t & idx )
{
	return new var_src_t( m_src, m_vars, src_id, idx, false );
}

void var_src_t::set( var_base_t * from )
{
	var_src_t * f = SRC( from );
	if( m_owner ) delete m_vars;
	m_src = f->m_src;
	m_vars = f->m_vars;
	f->m_owner = false;
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

void var_src_t::add_native_fn( const std::string & name, nativefnptr_t body, const size_t & args_count,
			      const bool is_va )
{
	m_vars->add( name, new var_fn_t( m_src->path(), "", is_va ? "." : "", std::vector< std::string >( args_count, "" ),
					 {}, { .native = body }, true, m_src->id(), 0 ), false );
}

void var_src_t::add_native_var( const std::string & name, var_base_t * val, const bool iref, const bool module_level )
{
	if( module_level ) m_vars->addm( name, val, iref );
	else m_vars->add( name, val, iref );
}

srcfile_t * var_src_t::src() { return m_src; }
vars_t * var_src_t::vars() { return m_vars; }
