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

size_t alloc_typeid()
{
	// skip VT_ALL
	static size_t id = 1;
	return id++;
}

var_base_t::var_base_t( const std::uintptr_t & type, const size_t & src_id, const size_t & idx,
			const bool & callable, const bool & attr_based )
	: m_type( type ), m_src_id( src_id ), m_idx( idx ), m_ref( 1 ),
	  m_callable( callable ), m_attr_based( attr_based )
{}
var_base_t::~var_base_t()
{}

std::uintptr_t var_base_t::typefn_id() const { return m_type; }

void * var_base_t::get_data( const size_t & idx ) { return nullptr; }

bool var_base_t::to_str( vm_state_t & vm, std::string & data, const size_t & src_id, const size_t & idx )
{
	var_base_t * str_fn = vm.get_typefn( this->type(), "str" );
	if( !str_fn ) {
		vm.fail( this->idx(), "no 'str' function implement for type: '%zu' or global type", this->type() );
		return false;
	}
	if( !str_fn->call( vm, { this }, {}, {}, src_id, idx ) ) {
		vm.fail( this->idx(), "function call 'str' for type: %zu failed", this->type() );
		return false;
	}
	var_base_t * str = vm.vm_stack->pop( false );
	if( !str->istype< var_str_t >() ) {
		vm.fail( this->idx(), "expected string return type from 'str' function, received: %s",
			 vm.type_name( str ).c_str() );
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
	vm.set_typename( type_id< var_nil_t >(), "nil" );
	vm.set_typename( type_id< var_typeid_t >(), "typeid" );
	vm.set_typename( type_id< var_bool_t >(), "bool" );
	vm.set_typename( type_id< var_int_t >(), "int" );
	vm.set_typename( type_id< var_flt_t >(), "float" );
	vm.set_typename( type_id< var_str_t >(), "string" );
	vm.set_typename( type_id< var_vec_t >(), "vector" );
	vm.set_typename( type_id< var_map_t >(), "map" );
	vm.set_typename( type_id< var_fn_t >(), "function" );
	vm.set_typename( type_id< var_file_t >(), "file" );
	vm.set_typename( type_id< var_struct_def_t >(), "struct_def" );
	vm.set_typename( type_id< var_src_t >(), "module" );

	vm.gadd( "nil_t", make< var_typeid_t >( type_id< var_nil_t >() ) );
	vm.gadd( "typeid_t", make< var_typeid_t >( type_id< var_typeid_t >() ) );
	vm.gadd( "bool_t", make< var_typeid_t >( type_id< var_bool_t >() ) );
	vm.gadd( "int_t", make< var_typeid_t >( type_id< var_int_t >() ) );
	vm.gadd( "flt_t", make< var_typeid_t >( type_id< var_flt_t >() ) );
	vm.gadd( "str_t", make< var_typeid_t >( type_id< var_str_t >() ) );
	vm.gadd( "vec_t", make< var_typeid_t >( type_id< var_vec_t >() ) );
	vm.gadd( "map_t", make< var_typeid_t >( type_id< var_map_t >() ) );
	vm.gadd( "func_t", make< var_typeid_t >( type_id< var_fn_t >() ) );
	vm.gadd( "file_t", make< var_typeid_t >( type_id< var_file_t >() ) );
	vm.gadd( "struct_def_t", make< var_typeid_t >( type_id< var_struct_def_t >() ) );
	vm.gadd( "src_t", make< var_typeid_t >( type_id< var_src_t >() ) );
}
