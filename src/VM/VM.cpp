/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <string>
#include <cstdlib>

#include "../Common/String.hpp"
#include "../Common/Env.hpp"
#include "../Common/FS.hpp"
#include "Vars.hpp"
#include "VM.hpp"

vm_state_t::vm_state_t( const size_t & flags )
	: exec_flags( flags ), tru( new var_bool_t( true, 0 ) ),
	  fals( new var_bool_t( false, 0 ) ), nil( new var_nil_t( 0 ) ),
	  vm_stack( new vm_stack_t() ), dlib( new dyn_lib_t() )
{
	init_builtin_types( * this );
	inc_locs.emplace_back( STRINGIFY( BUILD_PREFIX_DIR ) "/include/feral" );
	mod_locs.emplace_back( STRINGIFY( BUILD_PREFIX_DIR ) "/lib/feral" );
}

vm_state_t::~vm_state_t()
{
	delete vm_stack;
	for( auto & t : m_types ) var_dref( t.second );
	for( auto & g : m_globals ) var_dref( g.second );
	for( auto & src : all_srcs ) var_dref( src.second );
	var_dref( nil );
	var_dref( fals );
	var_dref( tru );
	delete dlib;
}

void vm_state_t::add_src( srcfile_t * src, const size_t & idx )
{
	if( all_srcs.find( src->get_path() ) == all_srcs.end() ) {
		all_srcs[ src->get_path() ] = new var_module_t( src, new srcfile_vars_t(), idx );
	}
	var_iref( all_srcs[ src->get_path() ] );
	src_stack.push_back( all_srcs[ src->get_path() ] );
}

void vm_state_t::pop_src() { var_dref( src_stack.back() ); src_stack.pop_back(); }

void vm_state_t::sadd( var_struct_t * sd, const bool iref )
{
	if( m_types.find( sd->id() ) != m_types.end() ) return;
	if( iref ) var_iref( sd );
	m_types[ sd->id() ] = sd;
}

var_struct_t * vm_state_t::sget( var_base_t * of )
{
	if( of->type() < VT_STRUCT ) {
		if( m_types.find( of->type() ) == m_types.end() ) return nullptr;
		return m_types[ of->type() ];
	}
	return STRUCT( of );
}

var_base_t * vm_state_t::sattr( var_base_t * of, const std::string & name )
{
	var_struct_t * st = nullptr;
	if( of->type() < VT_STRUCT ) {
		st = m_types[ of->type() ];
	} else {
		st = STRUCT( of );
	}
	var_base_t * attr = st->get_attr( name );
	if( attr ) return attr;

	const std::vector< size_t > & inherits = st->inherit_chain();
	for( auto & inh : inherits ) {
		attr = m_types[ inh ]->get_attr( name );
		if( attr ) return attr;
	}

	return attr;
}

void vm_state_t::saddattr( const size_t & id, const std::string & name, var_base_t * attr, const bool iref )
{
	if( m_types.find( id ) == m_types.end() ) {
		if( !iref ) var_dref( attr );
		return;
	}
	m_types[ id ]->add_attr( name, attr, iref );
}

void vm_state_t::gadd( const std::string & name, var_base_t * val, const bool iref )
{
	if( m_globals.find( name ) != m_globals.end() ) return;
	if( iref ) var_iref( val );
	m_globals[ name ] = val;
}

var_base_t * vm_state_t::gget( const std::string & name )
{
	if( m_globals.find( name ) == m_globals.end() ) return nullptr;
	return m_globals[ name ];
}

bool vm_state_t::mod_exists( const std::vector< std::string > & locs, std::string & mod, const std::string & ext )
{
	if( mod.front() != '~' && mod.front() != '/' && mod.front() != '.' ) {
		for( auto & loc : locs ) {
			if( fs::exists( loc + "/" + mod + ext ) ) {
				mod = loc + "/" + mod + ext;
				return true;
			}
		}
	} else {
		if( mod.front() == '~' ) {
			mod.erase( mod.begin() );
			std::string home = env::get( "HOME" );
			mod.insert( mod.begin(), home.begin(), home.end() );
		}
		return fs::exists( mod );
	}
	return false;
}

bool vm_state_t::load_nmod( const std::string & mod_str, const size_t & idx )
{
	std::string mod = mod_str.substr( mod_str.find_last_of( '/' ) + 1 );
	std::string mod_file = mod_str;
	mod_file.insert( mod_file.find_last_of( '/' ) + 1, "lib" );
	srcfile_t * src = src_stack.back()->src();
	if( !mod_exists( mod_locs, mod_file, nmod_ext() ) ) {
		src->fail( idx, "module file: %s not found in locations: %s",
			   ( mod_file + nmod_ext() ).c_str(), str::stringify( mod_locs ).c_str() );
		return false;
	}

	if( dlib->fexists( mod_file ) ) return true;

	if( !dlib->load( mod_file ) ) {
		src->fail( idx, "unable to load module file: %s",
			   mod_file.c_str(), str::stringify( mod_locs ).c_str() );
		return false;
	}
	mod_init_fn_t init_fn = ( mod_init_fn_t )dlib->get( mod_file, "init_" + mod );
	if( init_fn == nullptr ) {
		src->fail( idx, "module file: %s does not contain init function (%s)",
			   mod_file.c_str(), ( "init_" + mod ).c_str() );
		dlib->unload( mod_file );
		return false;
	}
	if( !init_fn( * this ) ) {
		dlib->unload( mod_file );
		src->fail( idx, "init function in module file: %s didn't return okay",
			   mod_file.c_str() );
		return false;
	}
	return true;
}

int vm_state_t::load_fmod( const std::string & mod_file )
{
	if( all_srcs.find( mod_file ) != all_srcs.end() ) return E_OK;

	Errors err = E_OK;
	srcfile_t * src = m_src_load_fn( mod_file, exec_flags, true, err );
	if( err != E_OK ) {
		if( src ) delete src;
		return err;
	}

	add_src( src, 0 );
	int res = vm::exec( * this, src->bcode() );
	pop_src();
	return res;
}

bool vm_state_t::load_core_mods()
{
	std::vector< std::string > mods = { "core" };
	for( auto & mod : mods ) {
		if( !load_nmod( mod, 0 ) ) return false;
	}
	return true;
}


const char * nmod_ext()
{
#ifdef __linux__
	return ".so";
#elif __APPLE__
	return ".dylib";
#endif
}

const char * fmod_ext()
{
	return ".fer";
}
