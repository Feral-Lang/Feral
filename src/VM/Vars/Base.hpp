/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef VM_VARS_BASE_HPP
#define VM_VARS_BASE_HPP

#include <cassert>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <gmpxx.h>

#include "../../../third_party/mpfrxx.hpp"

enum VarTypes
{
	VT_NIL,

	VT_BOOL,
	VT_INT,
	VT_FLT,
	VT_STR,

	VT_FUNC,

	VT_STRUCT,

	// all custom types have typeid >= _VT_LAST
	_VT_LAST,
};

class var_base_t
{
	size_t m_type;
	size_t m_idx;
	size_t m_ref;
public:
	var_base_t( const size_t & type, const size_t & idx, const size_t & ref );
	virtual ~var_base_t();

	virtual var_base_t * copy( const size_t & idx ) = 0;
	virtual void set( var_base_t * from ) = 0;

	inline size_t type() const { return m_type; }

	inline size_t idx() const { return m_idx; }

	inline void iref() { ++m_ref; }
	inline size_t dref() { assert( m_ref > 0 ); --m_ref; return m_ref; }
	inline size_t ref() const { return m_ref; }

	static void * operator new( size_t sz );
	static void operator delete( void * ptr, size_t sz );
};

template< typename T > inline void var_iref( T * & var )
{
	if( var == nullptr ) return;
	var->iref();
}
template< typename T > inline void var_dref( T * & var )
{
	if( var == nullptr ) return;
	var->dref();
	if( var->ref() == 0 ) {
		delete var;
		var = nullptr;
	}
}

class var_nil_t : public var_base_t
{
public:
	var_nil_t( const size_t & idx );

	var_base_t * copy( const size_t & idx );
	void set( var_base_t * from );
};
#define NIL( x ) static_cast< var_nil_t * >( x )

class var_bool_t : public var_base_t
{
	bool m_val;
public:
	var_bool_t( const bool val, const size_t & idx );

	var_base_t * copy( const size_t & idx );
	bool & get();
	void set( var_base_t * from );
};
#define BOOL( x ) static_cast< var_bool_t * >( x )

class var_int_t : public var_base_t
{
	mpz_class m_val;
public:
	var_int_t( const mpz_class & val, const size_t & idx );

	var_base_t * copy( const size_t & idx );
	mpz_class & get();
	void set( var_base_t * from );
};
#define INT( x ) static_cast< var_int_t * >( x )

class var_flt_t : public var_base_t
{
	mpfr::mpreal m_val;
public:
	var_flt_t( const mpfr::mpreal & val, const size_t & idx );

	var_base_t * copy( const size_t & idx );
	mpfr::mpreal & get();
	void set( var_base_t * from );
};
#define FLT( x ) static_cast< var_flt_t * >( x )

class var_str_t : public var_base_t
{
	std::string m_val;
public:
	var_str_t( const std::string & val, const size_t & idx );

	var_base_t * copy( const size_t & idx );
	std::string & get();
	void set( var_base_t * from );
};
#define STR( x ) static_cast< var_str_t * >( x )

struct fn_body_span_t
{
	size_t begin;
	size_t end;
};

struct vm_state_t;
struct func_call_data_t;
typedef var_base_t * ( * nativefnptr_t )( vm_state_t & vm, func_call_data_t & fcd );

union fn_body_t
{
	nativefnptr_t native;
	fn_body_span_t feral;
};

class var_fn_t : public var_base_t
{
	size_t m_fn_id;
	std::string m_kw_arg;
	std::string m_var_arg;
	std::vector< std::string > m_args_order;
	std::unordered_map< std::string, var_base_t * > m_args;
	fn_body_t m_body;
	bool m_is_native;
public:
	var_fn_t( const std::string & kw_arg, const std::string & var_arg,
		  const std::vector< std::string > & args_order,
		  const std::unordered_map< std::string, var_base_t * > & args,
		  const fn_body_t & body, const bool is_native, const size_t & idx );

	~var_fn_t();

	var_base_t * copy( const size_t & idx );

	size_t & fn_id();
	std::string & kw_arg();
	std::string & var_arg();
	std::vector< std::string > & args_order();
	std::unordered_map< std::string, var_base_t * > & args();
	fn_body_t & body();
	bool is_native();

	void set( var_base_t * from );
};
#define FN( x ) static_cast< var_fn_t * >( x )

/* only used as base class for structures (not instantiated by itself) */
class var_struct_t : public var_base_t
{
	size_t m_id;
	std::unordered_set< size_t > m_inherits;
	std::unordered_map< std::string, var_base_t * > m_attrs;
public:
	var_struct_t( const size_t & id, const size_t & idx );
	var_struct_t( const size_t & idx );
	~var_struct_t();

	var_base_t * copy( const size_t & idx );

	size_t id();
	bool inherits( const size_t & id );
	void inherit( const size_t & id );
	bool add_attr( const std::string & name, var_base_t * val, const bool iref );
	var_base_t * get_attr( const std::string & name );
	bool has_attr( const std::string & name );

	void set( var_base_t * from );
};
#define STRUCT( x ) static_cast< var_struct_t * >( x )

void init_builtin_types( vm_state_t & vm );

#endif // VM_VARS_BASE_HPP
