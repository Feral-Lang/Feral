/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <algorithm>

#include "../Memory.hpp"
#include "../Vars.hpp"
#include "../VM.hpp"

#include "Base.hpp"

static size_t fn_id()
{
	static size_t fnid = 1;
	return fnid++;
}

static size_t struct_id()
{
	static size_t stid = _VT_LAST;
	return stid++;
}

var_base_t::var_base_t( const size_t & type, const size_t & idx, const size_t & ref )
	: m_type( type ), m_idx( idx ), m_ref( ref ) {}
var_base_t::~var_base_t() {}

var_nil_t::var_nil_t( const size_t & idx )
	: var_base_t( VT_NIL, idx, 1 ) {}

var_base_t * var_nil_t::copy( const size_t & idx ) { return new var_nil_t( idx ); }
void var_nil_t::set( var_base_t * from ) {}

var_bool_t::var_bool_t( const bool val, const size_t & idx )
	: var_base_t( VT_BOOL, idx, 1 ), m_val( val ) {}

var_base_t * var_bool_t::copy( const size_t & idx ) { return new var_bool_t( m_val, idx ); }
bool & var_bool_t::get() { return m_val; }
void var_bool_t::set( var_base_t * from )
{
	m_val = BOOL( from )->get();
}

var_int_t::var_int_t( const mpz_class & val, const size_t & idx )
	: var_base_t( VT_INT, idx, 1 ), m_val( val ) {}

var_base_t * var_int_t::copy( const size_t & idx ) { return new var_int_t( m_val, idx ); }
mpz_class & var_int_t::get() { return m_val; }
void var_int_t::set( var_base_t * from )
{
	m_val = INT( from )->get();
}

var_flt_t::var_flt_t( const mpfr::mpreal & val, const size_t & idx )
	: var_base_t( VT_FLT, idx, 1 ), m_val( val ) {}

var_base_t * var_flt_t::copy( const size_t & idx ) { return new var_flt_t( m_val, idx ); }
mpfr::mpreal & var_flt_t::get() { return m_val; }
void var_flt_t::set( var_base_t * from )
{
	m_val = FLT( from )->get();
}

var_str_t::var_str_t( const std::string & val, const size_t & idx )
	: var_base_t( VT_STR, idx, 1 ), m_val( val ) {}

var_base_t * var_str_t::copy( const size_t & idx ) { return new var_str_t( m_val, idx ); }
std::string & var_str_t::get() { return m_val; }
void var_str_t::set( var_base_t * from )
{
	m_val = STR( from )->get();
}

void * var_base_t::operator new( size_t sz )
{
	return mem::alloc( sz );
}
void var_base_t::operator delete( void * ptr, size_t sz )
{
	mem::free( ptr, sz );
}

var_fn_t::var_fn_t( const size_t & src_id, const std::string & kw_arg,
		    const std::string & var_arg, const std::vector< std::string > & args,
		    const std::vector< fn_assn_arg_t > & def_args,
		    const fn_body_t & body, const bool is_native, const size_t & idx )
	: var_base_t( VT_FUNC, idx, 1 ), m_fn_id( fn_id() ), m_src_id( src_id ), m_kw_arg( kw_arg ),
	  m_var_arg( var_arg ), m_args( args ), m_def_args( def_args ), m_body( body ),
	  m_is_native( is_native ) {}
var_fn_t::var_fn_t( const size_t & src_id, const std::vector< std::string > & args,
		    const fn_body_t & body, const size_t & idx )
	: var_base_t( VT_FUNC, idx, 1 ), m_fn_id( fn_id() ), m_src_id( src_id ),
	  m_args( args ), m_body( body ), m_is_native( true ) {}
var_fn_t::~var_fn_t()
{
	for( auto & arg : m_def_args ) var_dref( arg.val );
}

var_base_t * var_fn_t::copy( const size_t & idx )
{
	for( auto & arg : m_def_args ) var_iref( arg.val );
	return new var_fn_t( m_src_id, m_kw_arg, m_var_arg, m_args, m_def_args, m_body, m_is_native, idx );
}

size_t var_fn_t::fn_id() { return m_fn_id; }
size_t var_fn_t::src_id() { return m_src_id; }
std::string & var_fn_t::kw_arg() { return m_kw_arg; }
std::string & var_fn_t::var_arg() { return m_var_arg; }
std::vector< std::string > & var_fn_t::args() { return m_args; }
std::vector< fn_assn_arg_t > & var_fn_t::def_args() { return m_def_args; }
fn_body_t & var_fn_t::body() { return m_body; }
bool var_fn_t::is_native() { return m_is_native; }

bool var_fn_t::call( vm_state_t & vm, const std::vector< var_base_t * > & args,
		     const std::vector< fn_assn_arg_t > & assn_args,
		     const size_t & idx )
{
	if( m_is_native ) {
		var_base_t * res = m_body.native( vm, { idx, args, assn_args } );
		if( res == nullptr ) return false;
		vm.vm_stack->push_back( res );
		return true;
	}
	// take care of 'self' (always - data or nullptr)
	srcfile_t * src = vm.src_stack.back()->src();
	srcfile_vars_t * vars = vm.src_stack.back()->vars();
	vm_stack_t * vms = vm.vm_stack;
	return true;
}

void var_fn_t::set( var_base_t * from )
{
	var_fn_t * fn = FN( from );
	for( auto & arg : m_def_args ) var_dref( arg.val );

	for( auto & arg : fn->m_def_args ) var_dref( arg.val );

	// no need to change fn id
	m_src_id = fn->m_src_id;
	m_kw_arg = fn->m_kw_arg;
	m_var_arg = fn->m_var_arg;
	m_args = fn->m_args;
	m_def_args = fn->m_def_args;
	m_body = fn->m_body;
	m_is_native = fn->m_is_native;
}

var_module_t::var_module_t( srcfile_t * src, srcfile_vars_t * vars, const size_t & idx )
	: var_base_t( VT_MOD, idx, 1 ), m_src( src ), m_vars( vars ), m_copied( false ) {}
var_module_t::~var_module_t()
{
	if( !m_copied ) {
		delete m_vars;
		delete m_src;
	}
}

var_base_t * var_module_t::copy( const size_t & idx )
{
	m_copied = true;
	return new var_module_t( m_src, m_vars, idx );
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

var_struct_t::var_struct_t( const size_t & id, const size_t & idx )
	: var_base_t( VT_STRUCT, idx, 1 ), m_id( id )
{
	if( m_id != VT_ALL ) m_inherits.insert( m_inherits.begin(), VT_ALL );
	if( m_id != VT_ALL && m_id != VT_STRUCT ) m_inherits.insert( m_inherits.begin(), VT_STRUCT );
}
var_struct_t::var_struct_t( const size_t & idx )
	: var_base_t( VT_STRUCT, idx, 1 ), m_id( struct_id() )
{
	if( m_id != VT_ALL ) m_inherits.insert( m_inherits.begin(), VT_ALL );
	if( m_id != VT_ALL && m_id != VT_STRUCT ) m_inherits.insert( m_inherits.begin(), VT_STRUCT );
}
var_struct_t::~var_struct_t()
{
	for( auto & attr : m_attrs ) var_dref( attr.second );
}

var_base_t * var_struct_t::copy( const size_t & idx )
{
	for( auto & attr : m_attrs ) var_iref( attr.second );

	var_struct_t * res = new var_struct_t( m_id, idx );
	res->m_inherits = m_inherits;
	res->m_attrs = m_attrs;
	return res;
}

size_t var_struct_t::id() { return m_id; }
bool var_struct_t::inherits( const size_t & id ) { return std::find( m_inherits.begin(), m_inherits.end(), id ) != m_inherits.end(); }
void var_struct_t::inherit( const size_t & id )
{
	if( std::find( m_inherits.begin(), m_inherits.end(), id ) == m_inherits.end() ) {
		m_inherits.insert( m_inherits.begin(), id );
	}
}
bool var_struct_t::add_attr( const std::string & name, var_base_t * val, const bool iref )
{
	if( m_attrs.find( name ) != m_attrs.end() ) return false;
	if( iref ) var_iref( val );
	m_attrs[ name ] = val;
	return true;
}

var_base_t * var_struct_t::get_attr( const std::string & name )
{
	if( m_attrs.find( name ) == m_attrs.end() ) return nullptr;
	return m_attrs[ name ];
}
bool var_struct_t::has_attr( const std::string & name )
{
	return m_attrs.find( name ) != m_attrs.end();
}

const std::vector< size_t > & var_struct_t::inherit_chain() const { return m_inherits; }
const std::unordered_map< std::string, var_base_t * > & var_struct_t::attrs() const { return m_attrs; }

void var_struct_t::set( var_base_t * from )
{
	var_struct_t * st = STRUCT( from );
	for( auto & attr : m_attrs ) var_dref( attr.second );

	for( auto & attr : st->m_attrs ) var_iref( attr.second );

	m_id = st->m_id;
	m_inherits = st->m_inherits;
	m_attrs = st->m_attrs;
}

void init_builtin_types( vm_state_t & vm )
{
	vm.sadd( new var_struct_t( VT_ALL,  0 ) );
	vm.sadd( new var_struct_t( VT_NIL,  0 ) );
	vm.sadd( new var_struct_t( VT_BOOL, 0 ) );
	vm.sadd( new var_struct_t( VT_INT,  0 ) );
	vm.sadd( new var_struct_t( VT_FLT,  0 ) );
	vm.sadd( new var_struct_t( VT_STR,  0 ) );
	vm.sadd( new var_struct_t( VT_FUNC, 0 ) );
	vm.sadd( new var_struct_t( VT_MOD,  0 ) );
	vm.sadd( new var_struct_t( VT_STRUCT,  0 ) );
}