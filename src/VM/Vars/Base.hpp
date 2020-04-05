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

#include "../../Extra/mpfrxx.hpp"
#include "../SrcFile.hpp"

enum VarTypes
{
	VT_ALL,

	VT_NIL,

	VT_TYPEID,

	VT_BOOL,
	VT_INT,
	VT_FLT,
	VT_STR,
	VT_VEC,
	VT_MAP,

	VT_FUNC,

	VT_STRUCT_DEF,

	// all types from this inherit var_attr_based_t
	VT_SRC,

	// from this, struct type ids begin
	_VT_LAST,
};

struct vm_state_t;
// custom types inherit var_attr_based_t, NOT var_base_t
class var_base_t
{
protected:
	int m_type;
private:
	size_t m_src_id;
	size_t m_idx;
	size_t m_ref;
public:
	var_base_t( const int & type, const size_t & src_id, const size_t & idx );
	virtual ~var_base_t();

	// must always be overridden
	virtual var_base_t * copy( const size_t & src_id, const size_t & idx ) = 0;
	virtual void set( var_base_t * from ) = 0;

	// overridden by the type that needs it (mainly custom types like var_file_t)
	// returns nullptr by default
	// can use idx to specify which data to retrieve if multiple options are
	// available
	virtual void * get_data( const size_t & idx );

	bool to_str( vm_state_t & vm, std::string & data, const size_t & src_id, const size_t & idx );

	inline void set_src_id_idx( const size_t & src_id, const size_t & idx ) { m_src_id = src_id; m_idx = idx; }

	inline int type() const { return m_type; }

	inline size_t src_id() const { return m_src_id; }
	inline size_t idx() const { return m_idx; }

	inline void iref() { ++m_ref; }
	inline size_t dref() { assert( m_ref > 0 ); --m_ref; return m_ref; }
	inline size_t ref() const { return m_ref; }

	static void * operator new( size_t sz );
	static void operator delete( void * ptr, size_t sz );
};

template< typename T > inline void var_iref( T * var )
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
	var_nil_t( const size_t & src_id, const size_t & idx );

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );
};
#define NIL( x ) static_cast< var_nil_t * >( x )

class var_typeid_t : public var_base_t
{
	int m_val;
public:
	var_typeid_t( const int & val, const size_t & src_id, const size_t & idx );

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	int & get();
};
#define TYPEID( x ) static_cast< var_typeid_t * >( x )

class var_bool_t : public var_base_t
{
	bool m_val;
public:
	var_bool_t( const bool val, const size_t & src_id, const size_t & idx );

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	bool & get();
};
#define BOOL( x ) static_cast< var_bool_t * >( x )

class var_int_t : public var_base_t
{
	mpz_class m_val;
public:
	var_int_t( const mpz_class & val, const size_t & src_id, const size_t & idx );

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	mpz_class & get();
};
#define INT( x ) static_cast< var_int_t * >( x )

class var_flt_t : public var_base_t
{
	mpfr::mpreal m_val;
public:
	var_flt_t( const mpfr::mpreal & val, const size_t & src_id, const size_t & idx );

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	mpfr::mpreal & get();
};
#define FLT( x ) static_cast< var_flt_t * >( x )

class var_str_t : public var_base_t
{
	std::string m_val;
public:
	var_str_t( const std::string & val, const size_t & src_id, const size_t & idx );

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	std::string & get();
};
#define STR( x ) static_cast< var_str_t * >( x )

class var_vec_t : public var_base_t
{
	std::vector< var_base_t * > m_val;
public:
	var_vec_t( const std::vector< var_base_t * > & val, const size_t & src_id, const size_t & idx );
	~var_vec_t();

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	std::vector< var_base_t * > & get();
};
#define VEC( x ) static_cast< var_vec_t * >( x )

class var_map_t : public var_base_t
{
	std::unordered_map< std::string, var_base_t * > m_val;
public:
	var_map_t( const std::unordered_map< std::string, var_base_t * > & val, const size_t & src_id, const size_t & idx );
	~var_map_t();

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	std::unordered_map< std::string, var_base_t * > & get();
};
#define MAP( x ) static_cast< var_map_t * >( x )

struct fn_assn_arg_t
{
	size_t idx;
	std::string name;
	var_base_t * val;
};

struct fn_body_span_t
{
	size_t begin;
	size_t end;
};

struct fn_data_t
{
	size_t src_id;
	size_t idx;
	std::vector< var_base_t * > args;
	std::vector< fn_assn_arg_t > assn_args;
	std::unordered_map< std::string, size_t > assn_args_loc;
};

typedef var_base_t * ( * nativefnptr_t )( vm_state_t & vm, const fn_data_t & fd );

union fn_body_t
{
	nativefnptr_t native;
	fn_body_span_t feral;
};

class var_fn_t : public var_base_t
{
	std::string m_src_name;
	std::string m_kw_arg;
	std::string m_var_arg;
	std::vector< std::string > m_args;
	std::unordered_map< std::string, var_base_t * > m_assn_args;
	fn_body_t m_body;
	bool m_is_native;
public:
	var_fn_t( const std::string & src_name, const std::string & kw_arg,
		  const std::string & var_arg, const std::vector< std::string > & args,
		  const std::unordered_map< std::string, var_base_t * > & assn_args,
		  const fn_body_t & body, const bool is_native, const size_t & src_id,
		  const size_t & idx );
	var_fn_t( const std::string & src_name, const std::vector< std::string > & args,
		  const std::unordered_map< std::string, var_base_t * > & assn_args,
		  const fn_body_t & body, const size_t & src_id, const size_t & idx );

	~var_fn_t();

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	std::string & src_name();
	std::string & kw_arg();
	std::string & var_arg();
	std::vector< std::string > & args();
	std::unordered_map< std::string, var_base_t * > & assn_args();
	fn_body_t & body();
	bool is_native();

	bool call( vm_state_t & vm, const std::vector< var_base_t * > & args,
		   const std::vector< fn_assn_arg_t > & assn_args,
		   const std::unordered_map< std::string, size_t > & assn_args_loc,
		   const size_t & src_id, const size_t & idx );
};
#define FN( x ) static_cast< var_fn_t * >( x )

class var_struct_def_t : public var_base_t
{
	std::vector< std::string > m_attr_order;
	std::unordered_map< std::string, var_base_t * > m_attrs;
	// type id of struct which will be used as m_type for struct objects
	int m_id;

public:
	var_struct_def_t( const int & id, const std::vector< std::string > & attr_order,
			  const std::unordered_map< std::string, var_base_t * > & attrs,
			  const size_t & src_id, const size_t & idx );
	~var_struct_def_t();

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	// returns var_struct_t
	var_base_t * init( vm_state_t & vm, const std::vector< var_base_t * > & args,
			   const std::vector< fn_assn_arg_t > & assn_args,
			   const size_t & src_id, const size_t & idx );

	const std::vector< std::string > & attr_order() const;
	const std::unordered_map< std::string, var_base_t * > & attrs() const;
	size_t id() const;
};
#define STRUCT_DEF( x ) static_cast< var_struct_def_t * >( x )

class var_attr_based_t : public var_base_t
{
public:
	var_attr_based_t( const int & type, const size_t & src_id, const size_t & idx );
	virtual ~var_attr_based_t();

	virtual bool attr_exists( const std::string & name ) const = 0;
	virtual void attr_set( const std::string & name, var_base_t * val, const bool iref ) = 0;
	virtual var_base_t * attr_get( const std::string & name ) = 0;
};
#define ATTR_BASED( x ) static_cast< var_attr_based_t * >( x )

class vars_t;
class var_src_t : public var_attr_based_t
{
	srcfile_t * m_src;
	vars_t * m_vars;
	bool m_owner;
public:
	var_src_t( srcfile_t * src, vars_t * vars, const size_t & src_id, const size_t & idx, const bool owner = true );
	~var_src_t();

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	bool attr_exists( const std::string & name ) const;
	void attr_set( const std::string & name, var_base_t * val, const bool iref );
	var_base_t * attr_get( const std::string & name );

	void add_nativefn( const std::string & name, nativefnptr_t body,
			   const std::vector< std::string > & args = {},
			   const std::unordered_map< std::string, var_base_t * > & def_args = {},
			   const bool is_va = false );

	void add_nativevar( const std::string & name, var_base_t * val, const bool iref = true, const bool module_level = false );

	srcfile_t * src();
	vars_t * vars();
};
#define SRC( x ) static_cast< var_src_t * >( x )

class var_struct_t : public var_attr_based_t
{
	std::unordered_map< std::string, var_base_t * > m_attrs;
public:
	var_struct_t( const int & type_id, const std::unordered_map< std::string, var_base_t * > & attrs,
		      const size_t & src_id, const size_t & idx );
	~var_struct_t();

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	bool attr_exists( const std::string & name ) const;
	void attr_set( const std::string & name, var_base_t * val, const bool iref );
	var_base_t * attr_get( const std::string & name );

	const std::vector< std::string > & attr_order() const;
	const std::unordered_map< std::string, var_base_t * > & attrs() const;
};
#define STRUCT( x ) static_cast< var_struct_t * >( x )

void init_typenames( vm_state_t & vm );

#endif // VM_VARS_BASE_HPP
