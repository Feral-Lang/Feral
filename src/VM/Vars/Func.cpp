/*
	MIT License

	Copyright (c) 2020 Feral Language repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "VM/VM.hpp"
#include "VM/Vars/Base.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////// VAR_FN //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_fn_t::var_fn_t( const std::string & src_name, const std::string & kw_arg,
		    const std::string & var_arg, const std::vector< std::string > & args,
		    const std::unordered_map< std::string, var_base_t * > & assn_args,
		    const fn_body_t & body, const bool is_native, const size_t & src_id,
		    const size_t & idx )
	: var_base_t( type_id< var_fn_t >(), src_id, idx, true, false ), m_src_name( src_name ), m_kw_arg( kw_arg ),
	  m_var_arg( var_arg ), m_args( args ), m_assn_args( assn_args ), m_body( body ),
	  m_is_native( is_native )
{}
var_fn_t::var_fn_t( const std::string & src_name, const std::vector< std::string > & args,
		    const std::unordered_map< std::string, var_base_t * > & assn_args,
		    const fn_body_t & body, const size_t & src_id, const size_t & idx )
	: var_base_t( type_id< var_fn_t >(), src_id, idx, true, false ), m_src_name( src_name ), m_args( args ),
	  m_assn_args( assn_args ), m_body( body ), m_is_native( true )
{}
var_fn_t::~var_fn_t()
{
	for( auto & aa : m_assn_args ) var_dref( aa.second );
}

var_base_t * var_fn_t::copy( const size_t & src_id, const size_t & idx )
{
	for( auto & aa : m_assn_args ) {
		var_iref( aa.second );
	}
	return new var_fn_t( m_src_name, m_kw_arg, m_var_arg, m_args, m_assn_args, m_body, m_is_native, src_id, idx );
}

std::string & var_fn_t::src_name() { return m_src_name; }
std::string & var_fn_t::kw_arg() { return m_kw_arg; }
std::string & var_fn_t::var_arg() { return m_var_arg; }
std::vector< std::string > & var_fn_t::args() { return m_args; }
std::unordered_map< std::string, var_base_t * > & var_fn_t::assn_args() { return m_assn_args; }
fn_body_t & var_fn_t::body() { return m_body; }
bool var_fn_t::is_native() { return m_is_native; }

var_base_t * var_fn_t::call( vm_state_t & vm, const std::vector< var_base_t * > & args,
			     const std::vector< fn_assn_arg_t > & assn_args,
			     const std::unordered_map< std::string, size_t > & assn_args_loc,
			     const size_t & src_id, const size_t & idx )
{
	// - 1 for self
	if( args.size() - 1 < m_args.size() - m_assn_args.size() || ( args.size() - 1 > m_args.size() && m_var_arg.empty() ) ) {
		vm.fail( src_id, idx, "argument count required: %zu (without default args: %zu), received: %zu",
			 m_args.size(), m_args.size() - m_assn_args.size(), args.size() - 1 );
		return nullptr;
	}
	if( m_is_native ) {
		var_base_t * res = m_body.native( vm, { src_id, idx, args, assn_args, assn_args_loc } );
		if( res == nullptr ) return nullptr;
		// if it's a new variable (created with make<>() function)
		// set src_id and idx
		if( res->ref() == 0 ) {
			res->set_src_id_idx( src_id, idx );
		}
		vm.vm_stack->push( res );
		return vm.nil;
	}
	vm.push_src( m_src_name );
	vars_t * vars = vm.current_source()->vars();
	// take care of 'self' (always - data or nullptr)
	if( args[ 0 ] != nullptr ) {
		vars->stash( "self", args[ 0 ] );
	}
	// for default arguments
	std::unordered_set< std::string > found_args;
	size_t i = 1;
	for( auto & a : m_args ) {
		if( i == args.size() ) break;
		vars->stash( a, args[ i++ ] );
		found_args.insert( a );
	}
	// add all default arguments which have not been overwritten by args
	for( auto & aa : m_assn_args ) {
		if( found_args.find( aa.first ) != found_args.end() ) continue;
		var_base_t * copy = aa.second->copy( src_id, idx );
		copy->dref();
		vars->stash( aa.first, copy );
	}
	if( !m_var_arg.empty() ) {
		std::vector< var_base_t * > vec;
		while( i < args.size() ) {
			var_iref( args[ i ] );
			vec.push_back( args[ i ] );
			++i;
		}
		vars->stash( m_var_arg, make< var_vec_t >( vec, false ) );
	}
	if( !m_kw_arg.empty() ) {
		std::map< std::string, var_base_t * > map;
		for( auto & arg : assn_args ) {
			var_iref( arg.val );
			map[ arg.name ] = arg.val;
		}
		vars->stash( m_kw_arg, make< var_map_t >( map, false ) );
	}
	if( vm::exec( vm, nullptr, m_body.feral.begin, m_body.feral.end ) == E_EXEC_FAIL ) {
		goto fail;
	}
	vm.pop_src();
	return vm.nil;
fail:
	vars->unstash();
	vm.pop_src();
	return nullptr;
}

void var_fn_t::set( var_base_t * from )
{
	var_fn_t * fn = FN( from );

	for( auto & aa : m_assn_args ) var_dref( aa.second );

	for( auto & aa : fn->m_assn_args ) var_iref( aa.second );

	// no need to change fn id
	m_src_name = fn->m_src_name;
	m_kw_arg = fn->m_kw_arg;
	m_var_arg = fn->m_var_arg;
	m_args = fn->m_args;
	m_assn_args = fn->m_assn_args;
	m_body = fn->m_body;
	m_is_native = fn->m_is_native;
}
