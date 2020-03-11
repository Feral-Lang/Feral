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
////////////////////////////////////////////////////////////// VAR_FN //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_fn_t::var_fn_t( const std::string & src_name, const std::string & kw_arg,
		    const std::string & var_arg, const std::vector< std::string > & args,
		    const fn_body_t & body, const bool is_native,
		    const size_t & src_id, const size_t & idx )
	: var_base_t( VT_FUNC, src_id, idx ), m_src_name( src_name ), m_kw_arg( kw_arg ),
	  m_var_arg( var_arg ), m_args( args ), m_body( body ), m_is_native( is_native )
{}
var_fn_t::var_fn_t( const std::string & src_name, const std::vector< std::string > & args,
		    const fn_body_t & body, const size_t & src_id, const size_t & idx )
	: var_base_t( VT_FUNC, src_id, idx ), m_src_name( src_name ),
	  m_args( args ), m_body( body ), m_is_native( true )
{}
var_fn_t::~var_fn_t()
{
}

var_base_t * var_fn_t::copy( const size_t & src_id, const size_t & idx )
{
	return new var_fn_t( m_src_name, m_kw_arg, m_var_arg, m_args, m_body, m_is_native, src_id, idx );
}

std::string & var_fn_t::src_name() { return m_src_name; }
std::string & var_fn_t::kw_arg() { return m_kw_arg; }
std::string & var_fn_t::var_arg() { return m_var_arg; }
std::vector< std::string > & var_fn_t::args() { return m_args; }
fn_body_t & var_fn_t::body() { return m_body; }
bool var_fn_t::is_native() { return m_is_native; }

bool var_fn_t::call( vm_state_t & vm, const std::vector< var_base_t * > & args,
		     const std::vector< fn_assn_arg_t > & assn_args,
		     const size_t & src_id, const size_t & idx )
{
	// - 1 for self
	if( args.size() - 1 < m_args.size() || ( args.size() - 1 > m_args.size() && m_var_arg.empty() ) ) {
		vm.src_stack.back()->src()->fail( idx, "argument count required: %zu, received: %zu",
			   m_args.size(), args.size() - 1 );
		return false;
	}
	if( m_is_native ) {
		var_base_t * res = m_body.native( vm, { src_id, idx, args, assn_args } );
		if( res == nullptr ) return false;
		// if it's a new variable (created with make<>() function)
		// set src_id and idx
		if( res->ref() == 0 ) {
			res->set_src_id_idx( src_id, idx );
		}
		vm.vm_stack->push( res );
		return true;
	}
	vm.push_src( m_src_name );
	vars_t * vars = vm.src_stack.back()->vars();
	// take care of 'self' (always - data or nullptr)
	if( args[ 0 ] != nullptr ) {
		vars->stash( "self", args[ 0 ] );
	}
	size_t i = 1;
	for( auto & a : m_args ) {
		vars->stash( a, args[ i++ ] );
	}
	if( !m_var_arg.empty() ) {
		std::vector< var_base_t * > vec;
		while( i < args.size() ) {
			var_iref( args[ i ] );
			vec.push_back( args[ i ] );
			++i;
		}
		vars->stash( m_var_arg, make< var_vec_t >( vec ) );
	}
	if( !m_kw_arg.empty() ) {
		std::unordered_map< std::string, var_base_t * > map;
		for( auto & arg : assn_args ) {
			var_iref( arg.val );
			map[ arg.name ] = arg.val;
		}
		vars->stash( m_kw_arg, make< var_map_t >( map ) );
	}
	if( vm::exec( vm, m_body.feral.begin, m_body.feral.end ) == E_EXEC_FAIL ) {
		goto fail;
	}
	vm.pop_src();
	return true;
fail:
	vars->unstash();
	vm.pop_src();
	return false;
}

void var_fn_t::set( var_base_t * from )
{
	var_fn_t * fn = FN( from );

	// no need to change fn id
	m_src_name = fn->m_src_name;
	m_kw_arg = fn->m_kw_arg;
	m_var_arg = fn->m_var_arg;
	m_args = fn->m_args;
	m_body = fn->m_body;
	m_is_native = fn->m_is_native;
}
