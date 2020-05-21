/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "VM/Memory.hpp"
#include "VM/VM.hpp"

#include "VM/Vars/Base.hpp"

var_base_t::var_base_t( const std::uintptr_t & type, const size_t & src_id, const size_t & idx,
			const bool & callable, const bool & attr_based )
	: m_type( type ), m_src_id( src_id ), m_idx( idx ), m_ref( 1 ), m_info( 0 )
{
	if( callable ) m_info |= VI_CALLABLE;
	if( attr_based ) m_info |= VI_ATTR_BASED;
}
var_base_t::~var_base_t()
{}

std::uintptr_t var_base_t::typefn_id() const { return m_type; }

bool var_base_t::to_str( vm_state_t & vm, std::string & data, const size_t & src_id, const size_t & idx )
{
	var_base_t * str_fn = nullptr;
	if( attr_based() ) {
		str_fn = attr_get( "str" );
		if( str_fn == nullptr ) str_fn = vm.get_typefn( typefn_id(), "str", true );
	} else {
		str_fn = vm.get_typefn( typefn_id(), "str", false );
	}
	if( !str_fn ) {
		vm.fail( this->src_id(), this->idx(), "no 'str' function implement for type: '%zu' or global type", this->type() );
		return false;
	}
	if( !str_fn->call( vm, { this }, {}, {}, src_id, idx ) ) {
		vm.fail( this->src_id(), this->idx(), "function call 'str' for type: %zu failed", this->type() );
		return false;
	}
	var_base_t * str = vm.vm_stack->pop( false );
	if( !str->istype< var_str_t >() ) {
		vm.fail( this->src_id(), this->idx(), "expected string return type from 'str' function, received: %s",
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
	vm.register_type< var_all_t >( "all" );

	vm.register_type< var_nil_t >( "nil" );
	vm.register_type< var_typeid_t >( "typeid" );
	vm.register_type< var_bool_t >( "bool" );
	vm.register_type< var_int_t >( "int" );
	vm.register_type< var_flt_t >( "flt" );
	vm.register_type< var_str_t >( "str" );
	vm.register_type< var_vec_t >( "vec" );
	vm.register_type< var_map_t >( "map" );
	vm.register_type< var_fn_t >( "func" );
	vm.register_type< var_src_t >( "src" );
}
