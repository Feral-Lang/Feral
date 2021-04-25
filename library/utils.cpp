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

class var_int_iterable_t : public var_base_t
{
	mpz_t m_begin, m_end, m_step, m_curr;
	bool m_started;
	bool m_is_reverse;
public:
	var_int_iterable_t( const mpz_t begin, const mpz_t end, const mpz_t step,
			    const size_t & src_id, const size_t & idx );
	~var_int_iterable_t();

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	bool next( mpz_t & val );
};
#define INT_ITERABLE( x ) static_cast< var_int_iterable_t * >( x )

var_int_iterable_t::var_int_iterable_t( const mpz_t begin, const mpz_t end, const mpz_t step,
					const size_t & src_id, const size_t & idx )
	: var_base_t( type_id< var_int_iterable_t >(), src_id, idx, false, false ),
	  m_started( false ), m_is_reverse( mpz_cmp_si( step, 0 ) < 0 )
{
	mpz_init_set( m_begin, begin );
	mpz_init_set( m_end, end );
	mpz_init_set( m_step, step );
	mpz_init_set( m_curr, begin );
}
var_int_iterable_t::~var_int_iterable_t()
{
	mpz_clears( m_begin, m_end, m_step, m_curr, NULL );
}

var_base_t * var_int_iterable_t::copy( const size_t & src_id, const size_t & idx )
{
	return new var_int_iterable_t( m_begin, m_end, m_step, src_id, idx );
}
void var_int_iterable_t::set( var_base_t * from )
{
	mpz_set( m_begin, INT_ITERABLE( from )->m_begin );
	mpz_set( m_end, INT_ITERABLE( from )->m_end );
	mpz_set( m_step, INT_ITERABLE( from )->m_step );
	mpz_set( m_curr, INT_ITERABLE( from )->m_curr );
}

bool var_int_iterable_t::next( mpz_t & val )
{
	if( m_is_reverse ) {
		if( mpz_cmp( m_curr, m_end ) <= 0 ) return false;
	} else {
		if( mpz_cmp( m_curr, m_end ) >= 0 ) return false;
	}
	if( !m_started ) {
		mpz_init( val );
		mpz_set( val, m_curr );
		m_started = true;
		return true;
	}
	mpz_t tmp;
	mpz_init( tmp );
	mpz_add( tmp, m_curr, m_step );
	if( m_is_reverse ) {
		if( mpz_cmp( tmp, m_end ) <= 0 ) {
			mpz_clear( tmp );
			return false;
		}
	} else {
		if( mpz_cmp( tmp, m_end ) >= 0 ) {
			mpz_clear( tmp );
			return false;
		}
	}
	mpz_set( m_curr, tmp );
	mpz_init( val );
	mpz_set( val, m_curr );
	mpz_clear( tmp );
	return true;
}

var_base_t * range( vm_state_t & vm, const fn_data_t & fd )
{
	var_base_t * lhs_base = fd.args[ 1 ];
	var_base_t * rhs_base = fd.args.size() > 2 ? fd.args[ 2 ] : nullptr;
	var_base_t * step_base = fd.args.size() > 3 ? fd.args[ 3 ] : nullptr;

	if( !lhs_base->istype< var_int_t >() ) {
		vm.fail( lhs_base->src_id(), lhs_base->idx(), "expected argument 1 to be of type int, found: %s",
			 vm.type_name( lhs_base ).c_str() );
		return nullptr;
	}
	if( rhs_base && !rhs_base->istype< var_int_t >() ) {
		vm.fail( rhs_base->src_id(), rhs_base->idx(), "expected argument 2 to be of type int, found: %s",
			 vm.type_name( rhs_base ).c_str() );
		return nullptr;
	}
	if( step_base && !step_base->istype< var_int_t >() ) {
		vm.fail( step_base->src_id(), step_base->idx(), "expected argument 3 to be of type int, found: %s",
			 vm.type_name( step_base ).c_str() );
		return nullptr;
	}

	mpz_t begin, end, step;
	mpz_inits( begin, end, step, NULL );
	if( fd.args.size() > 2 ) mpz_set( begin, INT( lhs_base )->get() );
	else mpz_set_si( begin, 0 );
	if( rhs_base ) mpz_set( end, INT( rhs_base )->get() );
	else mpz_set( end, INT( lhs_base )->get() );
	if( step_base ) mpz_set( step, INT( step_base )->get() );
	else mpz_set_si( step, 1 );
	var_int_iterable_t * res = make< var_int_iterable_t >( begin, end, step );
	mpz_clears( begin, end, step, NULL );
	return res;
}

var_base_t * assertion( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_bool_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected boolean argument for assertion, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}

	if( !BOOL( fd.args[ 1 ] )->get() ) {
		vm.fail( fd.src_id, fd.idx, "assertion failed" );
		return nullptr;
	}
	return vm.nil;
}

var_base_t * int_iterable_next( vm_state_t & vm, const fn_data_t & fd )
{
	var_int_iterable_t * it = INT_ITERABLE( fd.args[ 0 ] );
	mpz_t _res;
	if( !it->next( _res ) ) return vm.nil;
	var_int_t * res = make< var_int_t >( _res );
	mpz_clear( _res );
	res->set_load_as_ref();
	return res;
}

INIT_MODULE( utils )
{
	const std::string & src_name = vm.current_source_file()->path();

	vm.gadd( "range", new var_fn_t( src_name, "", ".", { "" }, {}, { .native = range }, true, src_id, idx ), false );
	vm.gadd( "assert", new var_fn_t( src_name, "", "", { "" }, {}, { .native = assertion }, true, src_id, idx ), false );

	// get the type id for int iterable (register_type)
	vm.register_type< var_int_iterable_t >( "int_iterable_t", src_id, idx );

	vm.add_native_typefn< var_int_iterable_t >( "next", int_iterable_next, 0, src_id, idx );

	return true;
}
