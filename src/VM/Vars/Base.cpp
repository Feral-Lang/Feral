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

var_base_t::var_base_t( const int & type, const size_t & src_id, const size_t & idx,
			const bool & callable, const bool & attr_based )
	: m_type( type ), m_src_id( src_id ), m_idx( idx ), m_ref( 1 ),
	  m_callable( callable ), m_attr_based( attr_based )
{}
var_base_t::~var_base_t()
{}

size_t var_base_t::id() const { return m_type; }

void * var_base_t::get_data( const size_t & idx ) { return nullptr; }

bool var_base_t::to_str( vm_state_t & vm, std::string & data, const size_t & src_id, const size_t & idx )
{
	var_fn_t * str_fn = vm.get_typefn( this->type(), "str" );
	if( !str_fn ) {
		vm.fail( this->idx(), "no 'str' function implement for type: '%zu' or global type", this->type() );
		return false;
	}
	if( !FN( str_fn )->call( vm, { this }, {}, {}, src_id, idx ) ) {
		vm.fail( this->idx(), "function call 'str' for type: %zu failed", this->type() );
		return false;
	}
	var_base_t * str = vm.vm_stack->pop( false );
	if( str->type() != VT_STR ) {
		vm.fail( this->idx(), "expected string return type from 'str' function, received: %s",
			 vm.type_name( str->type() ).c_str() );
		var_dref( str );
		return false;
	}
	data = STR( str )->get();
	var_dref( str );
	return true;
}

var_base_t * var_base_t::call( vm_state_t & vm, const std::vector< var_base_t * > & args,
			       const std::vector< fn_assn_arg_t > & assn_args,
			       const std::unordered_map< std::string, size_t > & assn_args_loc,
			       const size_t & src_id, const size_t & idx )
{ return nullptr; }

bool var_base_t::attr_exists( const std::string & name ) const { return false; }
void var_base_t::attr_set( const std::string & name, var_base_t * val, const bool iref ) {}
var_base_t * var_base_t::attr_get( const std::string & name ) { return nullptr; }

void * var_base_t::operator new( size_t sz )
{
	return mem::alloc( sz );
}
void var_base_t::operator delete( void * ptr, size_t sz )
{
	mem::free( ptr, sz );
}

void init_typenames( vm_state_t & vm )
{
	vm.set_typename( VT_NIL, "nil" );
	vm.set_typename( VT_TYPEID, "typeid" );
	vm.set_typename( VT_BOOL, "bool" );
	vm.set_typename( VT_INT, "int" );
	vm.set_typename( VT_FLT, "float" );
	vm.set_typename( VT_STR, "string" );
	vm.set_typename( VT_VEC, "vector" );
	vm.set_typename( VT_MAP, "map" );
	vm.set_typename( VT_FUNC, "function" );
	vm.set_typename( VT_FILE, "file" );
	vm.set_typename( VT_STRUCT_DEF, "struct_def" );
	vm.set_typename( VT_SRC, "module" );

	vm.gadd( "nil_t", make< var_typeid_t >( VT_NIL ) );
	vm.gadd( "typeid_t", make< var_typeid_t >( VT_TYPEID ) );
	vm.gadd( "bool_t", make< var_typeid_t >( VT_BOOL ) );
	vm.gadd( "int_t", make< var_typeid_t >( VT_INT ) );
	vm.gadd( "flt_t", make< var_typeid_t >( VT_FLT ) );
	vm.gadd( "str_t", make< var_typeid_t >( VT_STR ) );
	vm.gadd( "vec_t", make< var_typeid_t >( VT_VEC ) );
	vm.gadd( "map_t", make< var_typeid_t >( VT_MAP ) );
	vm.gadd( "func_t", make< var_typeid_t >( VT_FUNC ) );
	vm.gadd( "file_t", make< var_typeid_t >( VT_FILE ) );
	vm.gadd( "struct_def_t", make< var_typeid_t >( VT_STRUCT_DEF ) );
	vm.gadd( "src_t", make< var_typeid_t >( VT_SRC ) );
}
