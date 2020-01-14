/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef COMPILER_PARSER_STMTS_HPP
#define COMPILER_PARSER_STMTS_HPP

#include <vector>

#include "../../Common/IO.hpp" // for disp() functions
#include "../Lex.hpp"

enum GramType
{
	GT_SIMPLE,
	GT_EXPR,
	GT_BLOCK,
	GT_VAR_DECL_BASE,
	GT_VAR_DECL_GLOBAL,
	GT_FN_ASSN_ARG,
	GT_FN_ARGS,
	GT_FN_DEF,
	GT_FN_CALL,
};

class stmt_base_t
{
	size_t m_idx;
	GramType m_type;
public:
	stmt_base_t( const GramType type, const size_t & idx );
	virtual ~stmt_base_t();

	virtual void disp( const bool has_next ) const = 0;

	size_t idx() const;
	GramType type() const;
};

typedef std::vector< const stmt_base_t * > ptree_t;

enum SimpleType
{
	ST_DATA,
	ST_OPER,

	ST_LAST,
};

extern const char * SimpleTypeStrs[ ST_LAST ];

class stmt_simple_t : public stmt_base_t
{
	SimpleType m_stype;
	const lex::tok_t * m_val;
public:
	stmt_simple_t( const SimpleType stype, const lex::tok_t * val );

	void disp( const bool has_next ) const;

	const lex::tok_t * val() const;
	SimpleType stype() const;
};

class stmt_block_t : public stmt_base_t
{
	std::vector< const stmt_base_t * > m_stmts;
public:
	stmt_block_t( const std::vector< const stmt_base_t * > & stmts, const size_t & idx );

	void disp( const bool has_next ) const;

	const std::vector< const stmt_base_t * > & stmts() const;
};

class stmt_expr_t : public stmt_base_t
{
	const stmt_base_t * m_lhs, * m_rhs;
	const lex::tok_t * m_oper;
	size_t m_commas;
public:
	stmt_expr_t( const stmt_base_t * lhs, const lex::tok_t * oper,
		     const stmt_base_t * rhs, const size_t & idx );
	~stmt_expr_t();

	void disp( const bool has_next ) const;

	const stmt_base_t * lhs() const;
	const stmt_base_t * rhs() const;
	const lex::tok_t * oper() const;
	size_t commas() const;
	void commas_set( const size_t & commas );
};

class stmt_var_decl_base_t : public stmt_base_t
{
	const stmt_base_t * m_lhs;
	const stmt_base_t * m_rhs;
public:
	stmt_var_decl_base_t( const stmt_base_t * lhs, const stmt_base_t * rhs );
	~stmt_var_decl_base_t();

	void disp( const bool has_next ) const;

	const stmt_base_t * lhs() const;
	const stmt_base_t * rhs() const;
};

enum VarDeclType
{
	VDT_GLOBAL,
	VDT_LOCAL,
};

class stmt_var_decl_t : public stmt_base_t
{
	VarDeclType m_dtype;
	const std::vector< const stmt_var_decl_base_t * > m_decls;
public:
	stmt_var_decl_t( const VarDeclType dtype, const std::vector< const stmt_var_decl_base_t * > & decls, const size_t & idx );
	~stmt_var_decl_t();

	void disp( const bool has_next ) const;

	VarDeclType dtype() const;
	const std::vector< const stmt_var_decl_base_t * > & decls() const;
};

class stmt_fn_assn_arg_t : public stmt_base_t
{
	const lex::tok_t * m_lhs;
	const stmt_base_t * m_rhs;
public:
	stmt_fn_assn_arg_t( const lex::tok_t * lhs, const stmt_base_t * rhs );
	~stmt_fn_assn_arg_t();

	void disp( const bool has_next ) const;

	const lex::tok_t * lhs() const;
	const stmt_base_t * rhs() const;
};

class stmt_fn_def_args_t : public stmt_base_t
{
	const std::vector< const stmt_base_t * > m_args;
	const lex::tok_t * m_kwarg, * m_vaarg;
public:
	stmt_fn_def_args_t( const std::vector< const stmt_base_t * > & args,
			    const lex::tok_t * kwarg, const lex::tok_t * vaarg,
			    const size_t & idx );
	~stmt_fn_def_args_t();

	void disp( const bool has_next ) const;

	const std::vector< const stmt_base_t * > & args() const;
	const lex::tok_t * kwarg() const;
	const lex::tok_t * vaarg() const;
};

class stmt_fn_def_t : public stmt_base_t
{
	const stmt_fn_def_args_t * m_args;
	const stmt_block_t * m_block;
public:
	stmt_fn_def_t( const stmt_fn_def_args_t * args, const stmt_block_t * block,
		       const size_t & idx );
	~stmt_fn_def_t();

	void disp( const bool has_next ) const;

	const stmt_fn_def_args_t * args() const;
	const stmt_block_t * block() const;
};

class stmt_fn_call_args_t : public stmt_base_t
{
	const std::vector< const stmt_base_t * > m_args, m_assn_args;
public:
	stmt_fn_call_args_t( const std::vector< const stmt_base_t * > & args,
			     const std::vector< const stmt_base_t * > & assn_args,
			     const size_t & idx );
	~stmt_fn_call_args_t();

	void disp( const bool has_next ) const;

	const std::vector< const stmt_base_t * > & args() const;
	const std::vector< const stmt_base_t * > & assn_args() const;
};

#endif // COMPILER_PARSER_STMTS_HPP
