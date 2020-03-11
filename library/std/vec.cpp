/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "../../src/VM/VM.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// Classes //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// initialize this in the init_utils function
static int vec_iterable_typeid;

class var_vec_iterable_t : public var_base_t
{
	var_vec_t * m_vec;
	size_t m_curr;
public:
	var_vec_iterable_t( var_vec_t * vec, const size_t & src_id, const size_t & idx );
	~var_vec_iterable_t();

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	bool next( var_base_t * & val );
};
#define VEC_ITERABLE( x ) static_cast< var_vec_iterable_t * >( x )

var_vec_iterable_t::var_vec_iterable_t( var_vec_t * vec, const size_t & src_id, const size_t & idx )
	: var_base_t( vec_iterable_typeid, src_id, idx ), m_vec( vec ), m_curr( 0 )
{
	var_iref( m_vec );
}
var_vec_iterable_t::~var_vec_iterable_t() { var_dref( m_vec ); }

var_base_t * var_vec_iterable_t::copy( const size_t & src_id, const size_t & idx )
{
	return new var_vec_iterable_t( m_vec, src_id, idx );
}
void var_vec_iterable_t::set( var_base_t * from )
{
	var_dref( m_vec );
	m_vec = VEC_ITERABLE( from )->m_vec;
	var_iref( m_vec );
	m_curr = VEC_ITERABLE( from )->m_curr;
}

bool var_vec_iterable_t::next( var_base_t * & val )
{
	if( m_curr >= m_vec->get().size() ) return false;
	val = m_vec->get()[ m_curr++ ];
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////// Functions /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_base_t * vec_new( vm_state_t & vm, const fn_data_t & fd )
{
	std::vector< var_base_t * > vec_val;
	for( size_t i = 1; i < fd.args.size(); ++i ) {
		vec_val.push_back( fd.args[ i ]->copy( fd.src_id, fd.idx ) );
	}
	return make< var_vec_t >( vec_val );
}

var_base_t * vec_size( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( VEC( fd.args[ 0 ] )->get().size() );
}

var_base_t * vec_empty( vm_state_t & vm, const fn_data_t & fd )
{
	return VEC( fd.args[ 0 ] )->get().size() == 0 ? vm.tru : vm.fals;
}

var_base_t * vec_each( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_vec_iterable_t >( VEC( fd.args[ 0 ] ) );
}

var_base_t * vec_iterable_next( vm_state_t & vm, const fn_data_t & fd )
{
	var_vec_iterable_t * it = VEC_ITERABLE( fd.args[ 0 ] );
	var_base_t * res = nullptr;
	if( !it->next( res ) ) return vm.nil;
	return res;
}

REGISTER_MODULE( vec )
{
	var_src_t * src = vm.src_stack.back();
	const std::string & src_name = src->src()->path();

	src->add_nativefn( "new", vec_new, {}, true );

	vm.add_typefn( VT_VEC,   "len", new var_fn_t( src_name, {}, { .native = vec_size  }, 0, 0 ), false );
	vm.add_typefn( VT_VEC, "empty", new var_fn_t( src_name, {}, { .native = vec_empty }, 0, 0 ), false );
	vm.add_typefn( VT_VEC,  "each", new var_fn_t( src_name, {}, { .native = vec_each  }, 0, 0 ), false );

	// get the type id for int iterable (register_type)
	vec_iterable_typeid = vm.register_new_type();

	vm.add_typefn( vec_iterable_typeid, "next", new var_fn_t( src_name, {}, { .native = vec_iterable_next }, 0, 0 ), false );

	return true;
}
