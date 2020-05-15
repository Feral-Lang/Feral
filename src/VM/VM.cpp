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
#include <cstdarg>

#include "Common/Env.hpp"
#include "Common/FS.hpp"
#include "Common/String.hpp"

#include "VM/Vars.hpp"
#include "VM/VM.hpp"

// env: FERAL_PATHS
vm_state_t::vm_state_t( const std::string & self_binary_loc, const std::vector< std::string > & args, const size_t & flags )
	: exit_called( false ), exit_code( 0 ), exec_flags( flags ),
	  tru( new var_bool_t( true, 0, 0 ) ), fals( new var_bool_t( false, 0, 0 ) ),
	  nil( new var_nil_t( 0, 0 ) ), vm_stack( new vm_stack_t() ),
	  dlib( new dyn_lib_t() ), m_self_binary( self_binary_loc ),
	  m_src_load_fn( nullptr ), m_src_read_code_fn( nullptr ),
	  m_prefix( STRINGIFY( BUILD_PREFIX_DIR ) )
{
	init_typenames( * this );

	std::vector< var_base_t * > src_args_vec;

	for( auto & arg : args ) {
		src_args_vec.push_back( new var_str_t( arg, 0, 0 ) );
	}
	src_args = new var_vec_t( src_args_vec, false, 0, 0 );

	std::vector< std::string > extra_search_paths = str::split( env::get( "FERAL_PATHS" ), ';' );

	for( auto & path : extra_search_paths ) {
		m_inc_locs.push_back( path + "/include/feral" );
		m_dll_locs.push_back( path + "/lib/feral" );
	}

	m_inc_locs.push_back( m_prefix + "/include/feral" );
	m_dll_locs.push_back( m_prefix + "/lib/feral" );
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

void vm_state_t::add_typefn( const std::uintptr_t & type, const std::string & name, var_base_t * fn, const bool iref )
{
	if( m_typefns.find( type ) == m_typefns.end() ) {
		m_typefns[ type ] = new vars_frame_t();
	}
	if( m_typefns[ type ]->exists( name ) ) {
		fprintf( stderr, "function '%s' for type '%s' already exists\n", name.c_str(),
			 type_name( type ).c_str() );
		assert( false );
		return;
	}
	m_typefns[ type ]->add( name, fn, iref );
}
var_base_t * vm_state_t::get_typefn( const std::uintptr_t & type, const std::string & name, const bool & all_only )
{
	if( all_only ) return m_typefns[ type_id< var_all_t >() ]->get( name );
	auto it = m_typefns.find( type );
	if( it == m_typefns.end() ) return m_typefns[ type_id< var_all_t >() ]->get( name );
	var_base_t * res = it->second->get( name );
	if( res ) return res;
	return FN( m_typefns[ type_id< var_all_t >() ]->get( name ) );
}

void vm_state_t::set_typename( const std::uintptr_t & type, const std::string & name )
{
	m_typenames[ type ] = name;
}
std::string vm_state_t::type_name( const std::uintptr_t & type )
{
	if( m_typenames.find( type ) != m_typenames.end() ) {
		return m_typenames[ type ];
	}
	return "typeid<" + std::to_string( type ) + ">";
}
std::string vm_state_t::type_name( const var_base_t * val )
{
	return type_name( val->type() );
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

bool vm_state_t::nmod_load( const std::string & mod_str, const size_t & src_id, const size_t & idx )
{
	std::string mod = mod_str.substr( mod_str.find_last_of( '/' ) + 1 );
	std::string mod_file = mod_str;
	mod_file.insert( mod_file.find_last_of( '/' ) + 1, "libferal" );
	if( !mod_exists( m_dll_locs, mod_file, nmod_ext() ) ) {
		fail( src_id, idx, "module file: %s not found in locations: %s",
		      ( mod_file + nmod_ext() ).c_str(), str::stringify( m_dll_locs ).c_str() );
		return false;
	}

	if( dlib->fexists( mod_file ) ) return true;

	if( !dlib->load( mod_file ) ) {
		fail( src_id, idx, "unable to load module file: %s",
		      mod_file.c_str(), str::stringify( m_dll_locs ).c_str() );
		return false;
	}
	mod_init_fn_t init_fn = ( mod_init_fn_t )dlib->get( mod_file, "init_" + mod );
	if( init_fn == nullptr ) {
		fail( src_id, idx, "module file: %s does not contain init function (%s)",
		      mod_file.c_str(), ( "init_" + mod ).c_str() );
		dlib->unload( mod_file );
		return false;
	}
	if( !init_fn( * this, src_id, idx ) ) {
		dlib->unload( mod_file );
		fail( src_id, idx, "init function in module file: %s didn't return okay",
		      mod_file.c_str() );
		return false;
	}
	// set deinit function if available
	mod_deinit_fn_t deinit_fn = ( mod_deinit_fn_t )dlib->get( mod_file, "deinit_" + mod );
	if( deinit_fn ) m_dll_deinit_fns[ mod_file ] = deinit_fn;
	return true;
}

int vm_state_t::fmod_load( const std::string & mod_file )
{
	if( all_srcs.find( mod_file ) != all_srcs.end() ) return E_OK;

	Errors err = E_OK;
	srcfile_t * src = m_src_load_fn( mod_file, exec_flags, false, err, 0, -1 );
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
		if( !nmod_load( mod, 0, 0 ) ) return false;
	}
	return true;
}

void vm_state_t::fail( const size_t & src_id, const size_t & idx, const char * msg, ... ) const
{
	va_list vargs;
	va_start( vargs, msg );
	for( auto & src : all_srcs ) {
		if( src.second->src()->id() == src_id ) {
			src.second->src()->fail( idx, msg, vargs );
		}
	}
	va_end( vargs );
}


const char * nmod_ext()
{
#if __linux__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__ || __bsdi__ || __DragonFly__
	return ".so";
#elif __APPLE__
	return ".dylib";
#endif
}

const char * fmod_ext( const bool compiled )
{
	if( compiled ) return ".cfer";
	return ".fer";
}