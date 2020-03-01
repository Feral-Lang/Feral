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

static var_base_t * contains_def_arg( const std::string & name, std::vector< fn_assn_arg_t > & assn_vars );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////// SOME EXTRAS ///////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static size_t fn_id()
{
	static size_t fnid = 1;
	return fnid++;
}

static size_t type_id()
{
	static size_t tid = VT_CUSTOM_START;
	return tid++;
}

// base instance for all other classes
// similar to python's object class
static std::unordered_map< std::string, var_base_t * > * var_all_base()
{
	static std::unordered_map< std::string, var_base_t * > v;
	return & v;
}
static std::unordered_map< std::string, var_base_t * > * var_nil_base()
{
	static std::unordered_map< std::string, var_base_t * > v;
	return & v;
}
static std::unordered_map< std::string, var_base_t * > * var_bool_base()
{
	static std::unordered_map< std::string, var_base_t * > v;
	return & v;
}
static std::unordered_map< std::string, var_base_t * > * var_int_base()
{
	static std::unordered_map< std::string, var_base_t * > v;
	return & v;
}
static std::unordered_map< std::string, var_base_t * > * var_flt_base()
{
	static std::unordered_map< std::string, var_base_t * > v;
	return & v;
}
static std::unordered_map< std::string, var_base_t * > * var_str_base()
{
	static std::unordered_map< std::string, var_base_t * > v;
	return & v;
}
static std::unordered_map< std::string, var_base_t * > * var_vec_base()
{
	static std::unordered_map< std::string, var_base_t * > v;
	return & v;
}
static std::unordered_map< std::string, var_base_t * > * var_map_base()
{
	static std::unordered_map< std::string, var_base_t * > v;
	return & v;
}
static std::unordered_map< std::string, var_base_t * > * var_fn_base()
{
	static std::unordered_map< std::string, var_base_t * > v;
	return & v;
}
static std::unordered_map< std::string, var_base_t * > * var_mod_base()
{
	static std::unordered_map< std::string, var_base_t * > v;
	return & v;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// VAR_BASE /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_base_t::var_base_t( const size_t & idx )
	: m_type( VT_ALL ), m_idx( idx ), m_ref( 1 )
{
	fuse( VT_ALL, var_all_base() );
}
var_base_t::var_base_t( const size_t & idx, const size_t & ref )
	: m_type( type_id() ), m_idx( idx ), m_ref( ref )
{
	fuse( VT_ALL, var_all_base() );
}
var_base_t::var_base_t( const size_t & type, const size_t & idx, const size_t & ref )
	: m_type( type ), m_idx( idx ), m_ref( ref )
{
	fuse( VT_ALL, var_all_base() );
}
var_base_t::~var_base_t()
{
	for( auto & a : m_attrs ) {
		var_dref( a.second );
	}
}

var_base_t * var_base_t::base_copy( const size_t & idx )
{
	var_base_t * cp = copy( idx );
	cp->m_type = m_type;
	cp->m_fused = m_fused;
	for( auto & a : m_attrs ) {
		if( cp->m_attrs.find( a.first ) != cp->m_attrs.end() ) {
			var_dref( cp->m_attrs[ a.first ] );
		}
		cp->m_attrs[ a.first ] = a.second->base_copy( idx );
	}
	return cp;
}

// base_set() calls set() at last for custom data assignment (besides fused & attrs)
void var_base_t::base_set( var_base_t * from )
{
	assert( m_type == from->m_type );
	m_type = from->m_type;
	m_idx = from->m_idx;
	m_fused = from->m_fused;
	for( auto & a : m_attrs ) {
		var_dref( a.second );
	}
	m_attrs.clear();
	for( auto & a : from->m_attrs ) {
		var_iref( a.second );
		m_attrs[ a.first ] = a.second;
	}
	set( from );
}

var_base_t * var_base_t::copy( const size_t & idx ) { return new var_base_t( idx ); }
void var_base_t::set( var_base_t * from ) {}

void var_base_t::fuse( var_base_t * what )
{
	if( m_fused.find( what->m_type ) != m_fused.end() ) return;
	m_fused.insert( what->m_type );
	for( auto & a : what->m_attrs ) {
		if( m_attrs.find( a.first ) != m_attrs.end() ) {
			var_dref( m_attrs[ a.first ] );
		}
		var_iref( a.second );
		m_attrs[ a.first ] = a.second;
	}
}
void var_base_t::fuse( const size_t & id, std::unordered_map< std::string, var_base_t * > * with )
{
	if( m_fused.find( id ) != m_fused.end() ) return;
	m_fused.insert( id );
	for( auto & a : * with ) {
		if( m_attrs.find( a.first ) != m_attrs.end() ) {
			var_dref( m_attrs[ a.first ] );
		}
		var_iref( a.second );
		m_attrs[ a.first ] = a.second;
	}
}
bool var_base_t::fused( const size_t & id ) { return m_fused.find( id ) != m_fused.end(); }
bool var_base_t::add_attr( const std::string & name, var_base_t * val, const bool iref )
{
	if( m_attrs.find( name ) != m_attrs.end() ) {
		if( iref ) var_dref( val );
		return false;
	}
	if( iref ) var_iref( val );
	m_attrs[ name ] = val;
	return true;
}

var_base_t * var_base_t::get_attr( const std::string & name )
{
	if( m_attrs.find( name ) == m_attrs.end() ) return nullptr;
	return m_attrs[ name ];
}
bool var_base_t::has_attr( const std::string & name )
{
	return m_attrs.find( name ) != m_attrs.end();
}

const std::unordered_set< size_t > & var_base_t::fuse_chain() const { return m_fused; }
const std::unordered_map< std::string, var_base_t * > & var_base_t::attrs() const { return m_attrs; }

void * var_base_t::operator new( size_t sz )
{
	return mem::alloc( sz );
}
void var_base_t::operator delete( void * ptr, size_t sz )
{
	mem::free( ptr, sz );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// VAR_NIL //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_nil_t::var_nil_t( const size_t & idx )
	: var_base_t( VT_NIL, idx, 1 ) { fuse( VT_NIL, var_nil_base() ); }

var_base_t * var_nil_t::copy( const size_t & idx ) { return new var_nil_t( idx ); }
void var_nil_t::set( var_base_t * from ) {}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// VAR_BOOL /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_bool_t::var_bool_t( const bool val, const size_t & idx )
	: var_base_t( VT_BOOL, idx, 1 ), m_val( val ) { fuse( VT_BOOL, var_bool_base() ); }

var_base_t * var_bool_t::copy( const size_t & idx ) { return new var_bool_t( m_val, idx ); }
bool & var_bool_t::get() { return m_val; }
void var_bool_t::set( var_base_t * from )
{
	m_val = BOOL( from )->get();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// VAR_INT //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_int_t::var_int_t( const mpz_class & val, const size_t & idx )
	: var_base_t( VT_INT, idx, 1 ), m_val( val ) { fuse( VT_INT, var_int_base() ); }

var_base_t * var_int_t::copy( const size_t & idx ) { return new var_int_t( m_val, idx ); }
mpz_class & var_int_t::get() { return m_val; }
void var_int_t::set( var_base_t * from )
{
	m_val = INT( from )->get();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// VAR_FLT //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_flt_t::var_flt_t( const mpfr::mpreal & val, const size_t & idx )
	: var_base_t( VT_FLT, idx, 1 ), m_val( val ) { fuse( VT_FLT, var_flt_base() ); }

var_base_t * var_flt_t::copy( const size_t & idx ) { return new var_flt_t( m_val, idx ); }
mpfr::mpreal & var_flt_t::get() { return m_val; }
void var_flt_t::set( var_base_t * from )
{
	m_val = FLT( from )->get();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// VAR_STR //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_str_t::var_str_t( const std::string & val, const size_t & idx )
	: var_base_t( VT_STR, idx, 1 ), m_val( val ) { fuse( VT_STR, var_str_base() ); }

var_base_t * var_str_t::copy( const size_t & idx ) { return new var_str_t( m_val, idx ); }
std::string & var_str_t::get() { return m_val; }
void var_str_t::set( var_base_t * from )
{
	m_val = STR( from )->get();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// VAR_VEC //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_vec_t::var_vec_t( const std::vector< var_base_t * > & val, const size_t & idx )
	: var_base_t( VT_VEC, idx, 1 ), m_val( val )
{
	fuse( VT_VEC, var_vec_base() );
}
var_vec_t::~var_vec_t()
{
	for( auto & v : m_val ) var_dref( v );
}

var_base_t * var_vec_t::copy( const size_t & idx )
{
	std::vector< var_base_t * > new_vec;
	for( auto & v : m_val ) {
		new_vec.push_back( v->base_copy( idx ) );
	}
	return new var_vec_t( new_vec, idx );
}
std::vector< var_base_t * > & var_vec_t::get() { return m_val; }
void var_vec_t::set( var_base_t * from )
{
	for( auto & v : m_val ) {
		var_dref( v );
	}
	m_val.clear();
	for( auto & v : VEC( from )->m_val ) {
		var_iref( v );
	}
	m_val = VEC( from )->m_val;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// VAR_MAP //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_map_t::var_map_t( const std::unordered_map< std::string, var_base_t * > & val, const size_t & idx )
	: var_base_t( VT_MAP, idx, 1 ), m_val( val )
{
	fuse( VT_MAP, var_map_base() );
}
var_map_t::~var_map_t()
{
	for( auto & v : m_val ) var_dref( v.second );
}

var_base_t * var_map_t::copy( const size_t & idx )
{
	std::unordered_map< std::string, var_base_t * > new_map;
	for( auto & v : m_val ) {
		new_map[ v.first ] =  v.second->base_copy( idx );
	}
	return new var_map_t( new_map, idx );
}
std::unordered_map< std::string, var_base_t * > & var_map_t::get() { return m_val; }
void var_map_t::set( var_base_t * from )
{
	for( auto & v : m_val ) {
		var_dref( v.second );
	}
	m_val.clear();
	for( auto & v : MAP( from )->m_val ) {
		var_iref( v.second );
	}
	m_val = MAP( from )->m_val;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////// VAR_FN //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_fn_t::var_fn_t( const std::string & src_name, const std::string & kw_arg,
		    const std::string & var_arg, const std::vector< std::string > & args,
		    const std::vector< fn_assn_arg_t > & def_args,
		    const fn_body_t & body, const bool is_native, const size_t & idx )
	: var_base_t( VT_FUNC, idx, 1 ), m_fn_id( fn_id() ), m_src_name( src_name ), m_kw_arg( kw_arg ),
	  m_var_arg( var_arg ), m_args( args ), m_def_args( def_args ), m_body( body ),
	  m_is_native( is_native )
{
	fuse( VT_FUNC, var_fn_base() );
}
var_fn_t::var_fn_t( const std::string & src_name, const std::vector< std::string > & args,
		    const fn_body_t & body, const size_t & idx )
	: var_base_t( VT_FUNC, idx, 1 ), m_fn_id( fn_id() ), m_src_name( src_name ),
	  m_args( args ), m_body( body ), m_is_native( true )
{
	fuse( VT_FUNC, var_fn_base() );
}
var_fn_t::~var_fn_t()
{
	for( auto & arg : m_def_args ) var_dref( arg.val );
}

var_base_t * var_fn_t::copy( const size_t & idx )
{
	for( auto & arg : m_def_args ) var_iref( arg.val );
	return new var_fn_t( m_src_name, m_kw_arg, m_var_arg, m_args, m_def_args, m_body, m_is_native, idx );
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
		     const size_t & idx )
{
	if( m_is_native ) {
		var_base_t * res = m_body.native( vm, { idx, args, assn_args } );
		if( res == nullptr ) return false;
		vm.vm_stack->push_back( res );
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

void init_builtin_types( vm_state_t & vm )
{
	vm.btadd( VT_ALL,  var_all_base() );
	vm.btadd( VT_NIL,  var_nil_base() );
	vm.btadd( VT_BOOL, var_bool_base() );
	vm.btadd( VT_INT,  var_int_base() );
	vm.btadd( VT_FLT,  var_flt_base() );
	vm.btadd( VT_STR,  var_str_base() );
	vm.btadd( VT_VEC,  var_vec_base() );
	vm.btadd( VT_MAP,  var_map_base() );
	vm.btadd( VT_FUNC, var_fn_base() );
	vm.btadd( VT_MOD,  var_mod_base() );
}