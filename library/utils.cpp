/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "../src/VM/VM.hpp"

// initialize this in the init_utils function
static int int_iterable_typeid;

class var_int_iterable_t : public var_base_t
{
	mpz_class m_begin, m_end, m_step, m_curr;
	bool m_started;
	bool m_is_reverse;
public:
	var_int_iterable_t( const mpz_class & begin, const mpz_class & end, const mpz_class & step,
			    const size_t & src_id, const size_t & idx );

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	bool next( mpz_class & val );
};
#define INT_ITERABLE( x ) static_cast< var_int_iterable_t * >( x )

var_int_iterable_t::var_int_iterable_t( const mpz_class & begin, const mpz_class & end, const mpz_class & step,
					const size_t & src_id, const size_t & idx )
	: var_base_t( int_iterable_typeid, src_id, idx ), m_begin( begin ), m_end( end ),
	  m_step( step ), m_curr( begin ), m_started( false ), m_is_reverse( step < 0 )
{}

var_base_t * var_int_iterable_t::copy( const size_t & src_id, const size_t & idx )
{
	return new var_int_iterable_t( m_begin, m_end, m_step, src_id, idx );
}
void var_int_iterable_t::set( var_base_t * from )
{
	m_begin = INT_ITERABLE( from )->m_begin;
	m_end = INT_ITERABLE( from )->m_end;
	m_step = INT_ITERABLE( from )->m_step;
	m_curr = INT_ITERABLE( from )->m_curr;
}

bool var_int_iterable_t::next( mpz_class & val )
{
	if( m_is_reverse ) {
		if( m_curr <= m_end ) return false;
	} else {
		if( m_curr >= m_end ) return false;
	}
	if( !m_started ) {
		val = m_curr;
		m_started = true;
		return true;
	}
	if( m_is_reverse ) {
		if( m_curr + m_step <= m_end ) return false;
	} else {
		if( m_curr + m_step >= m_end ) return false;
	}
	m_curr += m_step;
	val = m_curr;
	return true;
}

// TODO: type checking
// TODO: create var_flt_iterable_t
var_base_t * range( vm_state_t & vm, const fn_data_t & fd )
{
	var_base_t * lhs_base = fd.args[ 1 ];
	var_base_t * rhs_base = fd.args.size() > 2 ? fd.args[ 2 ] : nullptr;
	var_base_t * step_base = fd.args.size() > 3 ? fd.args[ 3 ] : nullptr;

	size_t final_type = VT_INT;

	if( lhs_base->type() == VT_FLT ) final_type = VT_FLT;
	if( rhs_base && rhs_base->type() == VT_FLT ) final_type = VT_FLT;
	if( step_base && step_base->type() == VT_FLT ) final_type = VT_FLT;

	if( final_type == VT_INT ) {
		mpz_class begin = fd.args.size() > 2 ? INT( lhs_base )->get() : 0;
		mpz_class end = rhs_base ? INT( rhs_base )->get() : INT( lhs_base )->get();
		mpz_class step = step_base ? INT( step_base )->get() : 1;
		return make< var_int_iterable_t >( begin, end, step );
	}
	return vm.nil;
}

var_base_t * assertion( vm_state_t & vm, const fn_data_t & fd )
{
	if( fd.args[ 1 ]->type() != VT_BOOL ) {
		vm.fail( fd.idx, "expected boolean argument for assertion, found: %s",
			 vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}

	if( !BOOL( fd.args[ 1 ] )->get() ) {
		vm.fail( fd.idx, "assertion failed" );
		return nullptr;
	}
	return vm.nil;
}

var_base_t * int_iterable_next( vm_state_t & vm, const fn_data_t & fd )
{
	var_int_iterable_t * it = INT_ITERABLE( fd.args[ 0 ] );
	mpz_class res;
	if( !it->next( res ) ) return vm.nil;
	return make< var_int_t >( res );
}

INIT_MODULE( utils )
{
	const std::string & src_name = vm.current_source_file()->path();

	vm.gadd( "range", new var_fn_t( src_name, "", ".", { "" }, {}, { .native = range }, true, src_id, idx ), false );
	vm.gadd( "assert", new var_fn_t( src_name, "", "", { "" }, {}, { .native = assertion }, true, src_id, idx ), false );

	// get the type id for int iterable (register_type)
	int_iterable_typeid = vm.register_new_type( "int_iterable_t", src_id, idx );

	vm.add_native_typefn( int_iterable_typeid, "next", int_iterable_next, 0, src_id, idx );

	return true;
}
