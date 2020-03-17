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

var_base_t * vec_front( vm_state_t & vm, const fn_data_t & fd )
{
	std::vector< var_base_t * > & vec = VEC( fd.args[ 0 ] )->get();
	return vec.size() == 0 ? vm.nil : vec.front();
}

var_base_t * vec_back( vm_state_t & vm, const fn_data_t & fd )
{
	std::vector< var_base_t * > & vec = VEC( fd.args[ 0 ] )->get();
	return vec.size() == 0 ? vm.nil : vec.back();
}

var_base_t * vec_push( vm_state_t & vm, const fn_data_t & fd )
{
	std::vector< var_base_t * > & vec = VEC( fd.args[ 0 ] )->get();
	var_iref( fd.args[ 1 ] );
	vec.push_back( fd.args[ 1 ] );
	return fd.args[ 0 ];
}

var_base_t * vec_pop( vm_state_t & vm, const fn_data_t & fd )
{
	std::vector< var_base_t * > & vec = VEC( fd.args[ 0 ] )->get();
	if( vec.size() > 0 ) {
		var_dref( vec.back() );
		vec.pop_back();
	}
	return fd.args[ 0 ];
}

var_base_t * vec_setat( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src_file = vm.src_stack.back()->src();
	if( fd.args[ 1 ]->type() != VT_INT ) {
		src_file->fail( fd.idx, "expected first argument to be of type integer for vec.set(), found: %s",
				vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	size_t pos = INT( fd.args[ 1 ] )->get().get_ui();
	std::vector< var_base_t * > & vec = VEC( fd.args[ 0 ] )->get();
	if( pos >= vec.size() ) {
		src_file->fail( fd.idx, "position %zu is not within string of length %zu",
				pos, vec.size() );
		return nullptr;
	}
	var_dref( vec[ pos ] );
	var_iref( fd.args[ 2 ] );
	vec[ pos ] = fd.args[ 2 ];
	return fd.args[ 0 ];
}

var_base_t * vec_insert( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src_file = vm.src_stack.back()->src();
	if( fd.args[ 1 ]->type() != VT_INT ) {
		src_file->fail( fd.idx, "expected first argument to be of type integer for string.insert(), found: %s",
				vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	size_t pos = INT( fd.args[ 1 ] )->get().get_ui();
	std::vector< var_base_t * > & vec = VEC( fd.args[ 0 ] )->get();
	if( pos > vec.size() ) {
		src_file->fail( fd.idx, "position %zu is greater than vector length %zu",
				pos, vec.size() );
		return nullptr;
	}
	var_iref( fd.args[ 2 ] );
	vec.insert( vec.begin() + pos, fd.args[ 2 ] );
	return fd.args[ 0 ];
}

var_base_t * vec_erase( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src_file = vm.src_stack.back()->src();
	if( fd.args[ 1 ]->type() != VT_INT ) {
		src_file->fail( fd.idx, "expected argument to be of type integer for string.erase(), found: %s",
				vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	size_t pos = INT( fd.args[ 1 ] )->get().get_ui();
	std::vector< var_base_t * > & vec = VEC( fd.args[ 0 ] )->get();
	if( pos < vec.size() ) vec.erase( vec.begin() + pos );
	return fd.args[ 0 ];
}

var_base_t * vec_last( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_int_t >( VEC( fd.args[ 0 ] )->get().size() - 1 );
}

var_base_t * vec_at( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src_file = vm.src_stack.back()->src();
	if( fd.args[ 1 ]->type() != VT_INT ) {
		src_file->fail( fd.idx, "expected argument to be of type integer for string.erase(), found: %s",
				vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	std::vector< var_base_t * > & vec = VEC( fd.args[ 0 ] )->get();
	size_t pos = INT( fd.args[ 1 ] )->get().get_ui();
	if( pos >= vec.size() ) return vm.nil;
	return vec[ pos ];
}

var_base_t * vec_iterable_next( vm_state_t & vm, const fn_data_t & fd )
{
	var_vec_iterable_t * it = VEC_ITERABLE( fd.args[ 0 ] );
	var_base_t * res = nullptr;
	if( !it->next( res ) ) return vm.nil;
	return res;
}

INIT_MODULE( vec )
{
	var_src_t * src = vm.src_stack.back();
	const std::string & src_name = src->src()->path();

	src->add_nativefn( "new", vec_new, {}, {}, true );

	vm.add_typefn( VT_VEC,   "len", new var_fn_t( src_name, {}, {}, { .native = vec_size  }, 0, 0 ), false );
	vm.add_typefn( VT_VEC, "empty", new var_fn_t( src_name, {}, {}, { .native = vec_empty }, 0, 0 ), false );
	vm.add_typefn( VT_VEC, "front", new var_fn_t( src_name, "",  "", {}, {}, { .native = vec_front }, true, 0, 0 ), false );
	vm.add_typefn( VT_VEC,  "back", new var_fn_t( src_name, "",  "", {}, {}, { .native = vec_back  }, true, 0, 0 ), false );
	vm.add_typefn( VT_VEC,  "push", new var_fn_t( src_name, "",  "", { "" }, {}, { .native = vec_push }, true, 0, 0 ), false );
	vm.add_typefn( VT_VEC,   "pop", new var_fn_t( src_name, "",  "", {}, {}, { .native = vec_pop   }, true, 0, 0 ), false );
	vm.add_typefn( VT_VEC, "insert", new var_fn_t( src_name, "",  "", { "", "" }, {}, { .native = vec_insert }, true, 0, 0 ), false );
	vm.add_typefn( VT_VEC, "erase", new var_fn_t( src_name, "",  "", { "" }, {}, { .native = vec_erase }, true, 0, 0 ), false );
	vm.add_typefn( VT_VEC, "lastidx", new var_fn_t( src_name, "",  "", {}, {}, { .native = vec_last }, true, 0, 0 ), false );
	vm.add_typefn( VT_VEC,   "set", new var_fn_t( src_name, "",  "", { "", "" }, {}, { .native = vec_setat }, true, 0, 0 ), false );
	vm.add_typefn( VT_VEC,    "at", new var_fn_t( src_name, "",  "", { "" }, {}, { .native = vec_at }, true, 0, 0 ), false );
	vm.add_typefn( VT_VEC,    "[]", new var_fn_t( src_name, "",  "", { "" }, {}, { .native = vec_at }, true, 0, 0 ), false );
	vm.add_typefn( VT_VEC,  "each", new var_fn_t( src_name, {}, {}, { .native = vec_each  }, 0, 0 ), false );

	// get the type id for int iterable (register_type)
	vec_iterable_typeid = vm.register_new_type( "var_vec_iterable_t", "vec_iterable_t" );

	vm.add_typefn( vec_iterable_typeid, "next", new var_fn_t( src_name, {}, {}, { .native = vec_iterable_next }, 0, 0 ), false );

	return true;
}
