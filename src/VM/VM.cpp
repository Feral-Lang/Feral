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

vm_state_t::vm_state_t( const std::string & self_binary_loc, const std::vector< std::string > & args, const size_t & flags )
	: exit_called( false ), exit_code( 0 ), exec_flags( flags ),
	  tru( new var_bool_t( true, 0, 0 ) ), fals( new var_bool_t( false, 0, 0 ) ),
	  nil( new var_nil_t( 0, 0 ) ), vm_stack( new vm_stack_t() ),
	  dlib( new dyn_lib_t() ), m_custom_types( -1 ), m_self_binary( self_binary_loc )
{
	init_typenames( * this );

	std::vector< var_base_t * > src_args_vec;

	for( auto & arg : args ) {
		src_args_vec.push_back( new var_str_t( arg, 0, 0 ) );
	}
	src_args = new var_vec_t( src_args_vec, 0, 0 );

	m_feral_home_dir = env::get("HOME") + "/.feral";

	m_inc_locs.emplace_back( m_feral_home_dir + "/include" );
	m_dll_locs.emplace_back( m_feral_home_dir + "/lib" );

	m_inc_locs.emplace_back( STRINGIFY( BUILD_PREFIX_DIR ) "/include/feral" );
	m_dll_locs.emplace_back( STRINGIFY( BUILD_PREFIX_DIR ) "/lib/feral" );
}

vm_state_t::~vm_state_t()
{
	delete vm_stack;
	for( auto & typefn : m_typefns ) delete typefn.second;
	for( auto & g : m_globals ) var_dref( g.second );
	for( auto & src : all_srcs ) var_dref( src.second );
	var_dref( nil );
	var_dref( fals );
	var_dref( tru );
	var_dref( src_args );
	for( auto & deinit_fn : m_dll_deinit_fns ) {
		deinit_fn.second();
	}
	delete dlib;
}

void vm_state_t::push_src( srcfile_t * src, const size_t & idx )
{
	if( all_srcs.find( src->path() ) == all_srcs.end() ) {
		all_srcs[ src->path() ] = new var_src_t( src, new vars_t(), src->id(), idx );
	}
	var_iref( all_srcs[ src->path() ] );
	src_stack.push_back( all_srcs[ src->path() ] );
}

void vm_state_t::push_src( const std::string & src_path )
{
	assert( all_srcs.find( src_path ) != all_srcs.end() );
	var_iref( all_srcs[ src_path ] );
	src_stack.push_back( all_srcs[ src_path ] );
}

void vm_state_t::pop_src() { var_dref( src_stack.back() ); src_stack.pop_back(); }

int vm_state_t::register_new_type( const std::string & name, const std::string & typeid_name )
{
	assert( m_dll_typenames.find( name ) == m_dll_typenames.end() );
	m_dll_typenames[ name ] = m_custom_types;
	// only add to source if it's not main source, else add to globals
	// for example, utils, core will be added to globals
	if( src_stack.size() > 1 ) {
		src_stack.back()->add_nativevar( typeid_name, make< var_typeid_t >( m_custom_types ), true, true );
	} else {
		assert( m_globals.find( typeid_name ) == m_globals.end() );
		m_globals[ typeid_name ] = new var_typeid_t( m_custom_types, 0, 0 );
	}
	return m_custom_types--;
}

int vm_state_t::dll_typeid( const std::string & name )
{
	if( m_dll_typenames.find( name ) == m_dll_typenames.end() ) return 0;
	return m_dll_typenames[ name ];
}

void vm_state_t::add_typefn( const int & type, const std::string & name, var_base_t * fn, const bool iref )
{
	if( m_typefns.find( type ) == m_typefns.end() ) {
		m_typefns[ type ] = new vars_frame_t();
	}
	m_typefns[ type ]->add( name, fn, iref );
}
var_fn_t * vm_state_t::get_typefn( const int & type, const std::string & name )
{
	if( m_typefns.find( type ) == m_typefns.end() ) return nullptr;
	return FN( m_typefns[ type ]->get( name ) );
}

void vm_state_t::set_typename( const int & type, const std::string & name )
{
	m_typenames[ type ] = name;
}
std::string vm_state_t::type_name( const int & type )
{
	if( m_typenames.find( type ) != m_typenames.end() ) {
		return m_typenames[ type ];
	}
	return "typeid<" + std::to_string( type ) + ">";
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
		} else if( mod.front() == '.' ) {
			// cannot have a module exists query with '.' outside all srcs
			assert( src_stack.size() > 0 );
			mod.erase( mod.begin() );
			mod = src_stack.back()->src()->dir() + mod;
		}
		if( fs::exists( mod + ext ) ) {
			mod = fs::abs_path( mod + ext );
			return true;
		}
	}
	return false;
}

bool vm_state_t::load_nmod( const std::string & mod_str, const size_t & idx, const bool set_dll_core_load_loc )
{
	std::string mod = mod_str.substr( mod_str.find_last_of( '/' ) + 1 );
	std::string mod_file = mod_str;
	mod_file.insert( mod_file.find_last_of( '/' ) + 1, "libferal" );
	srcfile_t * src = src_stack.back()->src();
	if( !mod_exists( m_dll_locs, mod_file, nmod_ext() ) ) {
		src->fail( idx, "module file: %s not found in locations: %s",
			   ( mod_file + nmod_ext() ).c_str(), str::stringify( m_dll_locs ).c_str() );
		return false;
	}

	if( set_dll_core_load_loc ) {
		m_dll_core_load_loc = mod_file.substr( 0, mod_file.find_last_of( '/' ) );
	}

	if( dlib->fexists( mod_file ) ) return true;

	if( !dlib->load( mod_file ) ) {
		src->fail( idx, "unable to load module file: %s",
			   mod_file.c_str(), str::stringify( m_dll_locs ).c_str() );
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
	// set deinit function if available
	mod_deinit_fn_t deinit_fn = ( mod_deinit_fn_t )dlib->get( mod_file, "deinit_" + mod );
	if( deinit_fn ) m_dll_deinit_fns[ mod_file ] = deinit_fn;
	return true;
}

int vm_state_t::load_fmod( const std::string & mod_file )
{
	if( all_srcs.find( mod_file ) != all_srcs.end() ) return E_OK;

	Errors err = E_OK;
	srcfile_t * src = m_src_load_fn( mod_file, exec_flags, false, err );
	if( err != E_OK ) {
		if( src ) delete src;
		return err;
	}

	push_src( src, 0 );
	int res = vm::exec( * this );
	pop_src();
	return res;
}

bool vm_state_t::load_core_mods()
{
	// TODO: perhaps embed these in feral binary to remove the requirement of installation location
	std::vector< std::string > mods = { "core", "utils" };
	for( auto & mod : mods ) {
		if( !load_nmod( mod, 0, mod == "core" ) ) return false;
	}
	return true;
}


const char * nmod_ext()
{
#if __linux__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__ || __bsdi__ || __DragonFly__
	return ".so";
#elif __APPLE__
	return ".dylib";
#endif
}

const char * fmod_ext()
{
	return ".fer";
}
