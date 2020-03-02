/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "../Memory.hpp"
#include "../VM.hpp"

#include "Base.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////// SOME EXTRAS ///////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static size_t type_id()
{
	static size_t tid = VT_CUSTOM_START;
	return tid++;
}

// base instance for all other classes
// similar to python's object class
std::unordered_map< std::string, var_base_t * > * var_all_base()
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
		m_attrs[ a.first ] = a.second->base_copy( a.second->idx() );
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

var_base_t * var_base_t::call_fn_result( vm_state_t & vm, const std::string & fn_name,
					 std::vector< var_base_t * > args, const size_t & idx )
{
	srcfile_t * src = vm.src_stack.back()->src();
	var_base_t * func = get_attr( fn_name );
	if( func == nullptr || func->type() != VT_FUNC ) {
		src->fail( idx, "type of this variable does not implement a '%s' function",
			   fn_name.c_str() );
		return nullptr;
	}

	args.insert( args.begin(), this );
	if( !FN( func )->call( vm, args, {}, idx ) ) {
		src->fail( idx, "failed to call '%s' function (make sure the argument count is correct)",
			   fn_name.c_str() );
		return nullptr;
	}
	var_base_t * data = vm.vm_stack->back();
	return data;
}

void * var_base_t::operator new( size_t sz )
{
	return mem::alloc( sz );
}
void var_base_t::operator delete( void * ptr, size_t sz )
{
	mem::free( ptr, sz );
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