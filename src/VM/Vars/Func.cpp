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

static size_t fn_id()
{
	// func id 0 = no func (global scope)
	static size_t fnid = 1;
	return fnid++;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////// VAR_FN //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_fn_t::var_fn_t( const std::string & src_name, const std::string & kw_arg,
		    const std::string & var_arg, const std::vector< std::string > & args,
		    const std::vector< fn_assn_arg_t > & def_args,
		    const fn_body_t & body, const bool is_native,
		    const size_t & src_id, const size_t & idx )
	: var_base_t( VT_FUNC, src_id, idx ), m_fn_id( fn_id() ), m_src_name( src_name ), m_kw_arg( kw_arg ),
	  m_var_arg( var_arg ), m_args( args ), m_def_args( def_args ), m_body( body ),
	  m_is_native( is_native )
{
}
var_fn_t::var_fn_t( const std::string & src_name, const std::vector< std::string > & args,
		    const fn_body_t & body, const size_t & src_id, const size_t & idx )
	: var_base_t( VT_FUNC, src_id, idx ), m_fn_id( fn_id() ), m_src_name( src_name ),
	  m_args( args ), m_body( body ), m_is_native( true )
{
}
var_fn_t::~var_fn_t()
{
	for( auto & arg : m_def_args ) var_dref( arg.val );
}

var_base_t * var_fn_t::copy( const size_t & src_id, const size_t & idx )
{
	for( auto & arg : m_def_args ) var_iref( arg.val );
	return new var_fn_t( m_src_name, m_kw_arg, m_var_arg, m_args, m_def_args, m_body, m_is_native, src_id, idx );
}

size_t var_fn_t::fn_id() const { return m_fn_id; }
std::string & var_fn_t::src_name() { return m_src_name; }
std::string & var_fn_t::kw_arg() { return m_kw_arg; }
std::string & var_fn_t::var_arg() { return m_var_arg; }
std::vector< std::string > & var_fn_t::args() { return m_args; }
std::vector< fn_assn_arg_t > & var_fn_t::def_args() { return m_def_args; }
fn_body_t & var_fn_t::body() { return m_body; }
bool var_fn_t::is_native() { return m_is_native; }

bool var_fn_t::call( vm_state_t & vm, const std::vector< var_base_t * > & args,
		     const std::vector< fn_assn_arg_t > & assn_args,
		     const size_t & src_id, const size_t & idx )
{
	if( m_is_native ) {
		// - 1 for self
		if( args.size() - 1 < m_args.size() || ( args.size() - 1 > m_args.size() && m_var_arg.empty() ) ) {
			vm.src_stack.back()->src()->fail( idx, "argument count required: %zu, received: %zu",
							  m_args.size(), args.size() - 1 );
			return false;
		}
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
	// take care of 'self' (always - data or nullptr)
	/*
	srcfile_t * src = vm.src_stack.back()->src();
	srcfile_vars_t * vars = vm.src_stack.back()->vars();
	vm_stack_t * vms = vm.vm_stack;
	vars->push_fn_id( m_fn_id );
	if( args[ 0 ] != nullptr ) {
		vars->stash( "self", args[ 0 ] );
	}
	for( size_t i = 0; i < m_args.size(); ++i ) {
		var_base_t * arg_val = nullptr;
		if( i >= args.size() ) {
			arg_val = contains_def_arg( m_args[ i ], m_def_args );
			if( arg_val == nullptr ) {
				src->fail( idx, "no default argument by name '%s' exists", m_args[ i ].c_str() );
				goto fail;
			}
		} else {
			arg_val = args[ i ];
		}
		vars->stash( m_args[ i ], arg_val );
	}
	vars->pop_fn_id();
	return true;
fail:
	vars->unstash();
	vars->pop_fn_id();
	return false;*/
	return true;
}

void var_fn_t::set( var_base_t * from )
{
	var_fn_t * fn = FN( from );
	for( auto & arg : m_def_args ) var_dref( arg.val );

	for( auto & arg : fn->m_def_args ) var_dref( arg.val );

	// no need to change fn id
	m_src_name = fn->m_src_name;
	m_kw_arg = fn->m_kw_arg;
	m_var_arg = fn->m_var_arg;
	m_args = fn->m_args;
	m_def_args = fn->m_def_args;
	m_body = fn->m_body;
	m_is_native = fn->m_is_native;
}

static var_base_t * contains_def_arg( const std::string & name, std::vector< fn_assn_arg_t > & assn_vars )
{
	for( auto & v : assn_vars ) {
		if( v.name == name ) return v.val;
	}
	return nullptr;
}
