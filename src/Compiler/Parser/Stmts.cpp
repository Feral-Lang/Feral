/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Stmts.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////// BASE //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

stmt_base_t::stmt_base_t( const GramType type, const size_t & idx )
	: m_idx( idx ), m_type( type ) {}
stmt_base_t::~stmt_base_t() {}

size_t stmt_base_t::idx() const { return m_idx; }
GramType stmt_base_t::type() const { return m_type; }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// SIMPLE /////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const char * SimpleTypeStrs[ ST_LAST ] = {
	"Data",
	"Operator",
};

stmt_simple_t::stmt_simple_t( const SimpleType stype, const lex::tok_t * val )
	: stmt_base_t( GT_SIMPLE, val->pos ), m_stype( stype ), m_val( val ) {}

const lex::tok_t * stmt_simple_t::val() const { return m_val; }
SimpleType stmt_simple_t::stype() const { return m_stype; }

void stmt_simple_t::disp( const bool has_next ) const
{
	io::tadd( has_next );
	io::print( has_next, "Simple at %p\n", this );
	io::tadd( false );
	io::print( false, "Value: %s (type: %s)\n",
		   m_stype == ST_DATA && !m_val->data.empty() ? m_val->data.c_str() : TokStrs[ m_val->type ],
		   m_stype == ST_DATA ? TokStrs[ m_val->type ] : SimpleTypeStrs[ m_stype ] );
	io::trem( 2 );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// BLOCK //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

stmt_block_t::stmt_block_t( const std::vector< const stmt_base_t * > & stmts, const size_t & idx )
	: stmt_base_t( GT_BLOCK, idx ), m_stmts( stmts ) {}

void stmt_block_t::disp( const bool has_next ) const
{
	io::tadd( has_next );
	io::print( has_next, "Block at %p\n", this );
	for( size_t i = 0; i < m_stmts.size(); ++i ) {
		m_stmts[ i ]->disp( i != m_stmts.size() - 1 );
	}
	io::trem();
}

const std::vector< const stmt_base_t * > & stmt_block_t::stmts() const { return m_stmts; }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////// EXPR //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

stmt_expr_t::stmt_expr_t( const stmt_base_t * lhs, const lex::tok_t * oper,
			  const stmt_base_t * rhs, const size_t & idx )
	: stmt_base_t( GT_EXPR, idx ), m_lhs( lhs ),
	  m_rhs( rhs ), m_oper( oper ), m_commas( 0 ) {}

stmt_expr_t::~stmt_expr_t()
{
	if( m_lhs ) delete m_lhs;
	if( m_rhs ) delete m_rhs;
}

const stmt_base_t * stmt_expr_t::lhs() const { return m_lhs; }
const stmt_base_t * stmt_expr_t::rhs() const { return m_rhs; }
const lex::tok_t * stmt_expr_t::oper() const { return m_oper; }
size_t stmt_expr_t::commas() const { return m_commas; }
void stmt_expr_t::commas_set( const size_t & commas ) { m_commas = commas; }

void stmt_expr_t::disp( const bool has_next ) const
{
	io::tadd( has_next );
	io::print( has_next, "Expression at: %p (commas: %zu)\n", this, m_commas );

	io::tadd( m_lhs != nullptr || m_rhs != nullptr );
	io::print( m_lhs != nullptr || m_rhs != nullptr,
		   "Operator: %s\n", m_oper != nullptr ? TokStrs[ m_oper->type ] : "(null)" );
	io::trem();
	io::tadd( m_rhs != nullptr );
	if( m_lhs != nullptr ) {
		io::print( m_rhs != nullptr, "LHS:\n" );
		m_lhs->disp( false );
	}
	io::trem();
	io::tadd( false );
	if( m_rhs != nullptr ) {
		io::print( false, "RHS:\n" );
		m_rhs->disp( false );
	}

	io::trem( 2 );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// VAR_DECL_BASE //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

stmt_var_decl_base_t::stmt_var_decl_base_t( const stmt_base_t * lhs, const stmt_base_t * rhs )
	: stmt_base_t( GT_VAR_DECL_BASE, lhs->idx() ), m_lhs( lhs ), m_rhs( rhs ) {}

stmt_var_decl_base_t::~stmt_var_decl_base_t()
{
	delete m_lhs;
	delete m_rhs;
}

void stmt_var_decl_base_t::disp( const bool has_next ) const
{
	io::tadd( has_next );
	io::print( has_next, "Var Decl Base at %p\n", this );
	io::tadd( true );
	io::print( true, "LHS:\n" );
	m_lhs->disp( false );
	io::trem();
	io::tadd( false );
	io::print( false, "RHS:\n" );
	m_rhs->disp( false );
	io::trem( 2 );
}

const stmt_base_t * stmt_var_decl_base_t::lhs() const { return m_lhs; }
const stmt_base_t * stmt_var_decl_base_t::rhs() const { return m_rhs; }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// VAR_DECL ///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

stmt_var_decl_t::stmt_var_decl_t( const VarDeclType dtype,
		const std::vector< const stmt_var_decl_base_t * > & decls,
		const size_t & idx )
	: stmt_base_t( GT_VAR_DECL_GLOBAL, idx ), m_dtype( dtype ), m_decls( decls ) {}

stmt_var_decl_t::~stmt_var_decl_t()
{ for( auto & decl : m_decls ) delete decl; }

void stmt_var_decl_t::disp( const bool has_next ) const
{
	io::tadd( has_next );
	io::print( has_next, "Var Decl(s) (%s) at %p\n",
		   m_dtype == VDT_GLOBAL ? "global" : "local", this );
	for( size_t i = 0; i < m_decls.size(); ++i ) {
		m_decls[ i ]->disp( i != m_decls.size() - 1 );
	}
	io::trem();
}

VarDeclType stmt_var_decl_t::dtype() const { return m_dtype; }
const std::vector< const stmt_var_decl_base_t * > & stmt_var_decl_t::decls() const { return m_decls; }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////// FUNC_ASSN_ARG /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

stmt_fn_assn_arg_t::stmt_fn_assn_arg_t( const lex::tok_t * lhs, const stmt_base_t * rhs )
	: stmt_base_t( GT_FN_ASSN_ARG, lhs->pos ), m_lhs( lhs ), m_rhs( rhs ) {}
stmt_fn_assn_arg_t::~stmt_fn_assn_arg_t()
{
	delete m_rhs;
}

void stmt_fn_assn_arg_t::disp( const bool has_next ) const
{
	io::tadd( has_next );
	io::print( has_next, "Assigned Argument at %p\n", this );
	io::tadd( true );
	io::print( m_rhs, "Parameter: %s\n", m_lhs->data.c_str() );
	io::trem();
	if( m_rhs ) {
		io::tadd( false );
		io::print( false, "Value:\n" );
		m_rhs->disp( false );
		io::trem();
	}
	io::trem();
}

const lex::tok_t * stmt_fn_assn_arg_t::lhs() const { return m_lhs; }
const stmt_base_t * stmt_fn_assn_arg_t::rhs() const { return m_rhs; }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// FUNC_DEF_ARGS //////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

stmt_fn_def_args_t::stmt_fn_def_args_t( const std::vector< const stmt_base_t * > & args,
					const lex::tok_t * kwarg, const lex::tok_t * vaarg,
					const size_t & idx )
	: stmt_base_t( GT_FN_ARGS, idx ), m_args( args ), m_kwarg( kwarg ), m_vaarg( vaarg ) {}
stmt_fn_def_args_t::~stmt_fn_def_args_t()
{
	for( auto & a : m_args ) delete a;
}

void stmt_fn_def_args_t::disp( const bool has_next ) const
{
	io::tadd( has_next );
	io::print( has_next, "Arguments at %p\n", this );
	if( m_kwarg ) {
		io::tadd( true );
		io::print( m_vaarg || m_args.size() > 0, "Keyword Argument: %s\n", m_kwarg->data.c_str() );
		io::trem();
	}
	if( m_vaarg ) {
		io::tadd( true );
		io::print( m_args.size() > 0, "Variadic Argument: %s\n", m_vaarg->data.c_str() );
		io::trem();
	}
	for( size_t i = 0; i < m_args.size(); ++i ) {
		m_args[ i ]->disp( i != m_args.size() - 1 );
	}
	io::trem();
}
const std::vector< const stmt_base_t * > & stmt_fn_def_args_t::args() const { return m_args; }
const lex::tok_t * stmt_fn_def_args_t::kwarg() const { return m_kwarg; }
const lex::tok_t * stmt_fn_def_args_t::vaarg() const { return m_vaarg; }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////// FUNC_DEF //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

stmt_fn_def_t::stmt_fn_def_t( const stmt_fn_def_args_t * args, const stmt_block_t * block,
			      const size_t & idx )
	: stmt_base_t( GT_FN_DEF, idx ), m_args( args ), m_block( block ) {}
stmt_fn_def_t::~stmt_fn_def_t()
{
	if( m_args ) delete m_args;
	delete m_block;
}

void stmt_fn_def_t::disp( const bool has_next ) const
{
	io::tadd( has_next );
	io::print( has_next, "Function definition at %p\n", this );
	if( m_args ) {
		m_args->disp( true );
	}
	m_block->disp( false );
	io::trem();
}
const stmt_fn_def_args_t * stmt_fn_def_t::args() const { return m_args; }
const stmt_block_t * stmt_fn_def_t::block() const { return m_block; }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////// FUNC_CALL_ARGS //////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

stmt_fn_call_args_t::stmt_fn_call_args_t( const std::vector< const stmt_base_t * > & args,
					  const std::vector< const stmt_base_t * > & assn_args,
					  const size_t & idx )
	: stmt_base_t( GT_FN_ARGS, idx ), m_args( args ), m_assn_args( assn_args ) {}
stmt_fn_call_args_t::~stmt_fn_call_args_t()
{
	for( auto & a : m_args ) delete a;
	for( auto & a : m_assn_args ) delete a;
}

void stmt_fn_call_args_t::disp( const bool has_next ) const
{
	io::tadd( has_next );
	io::print( has_next, "Arguments at %p\n", this );
	for( size_t i = 0; i < m_args.size(); ++i ) {
		m_args[ i ]->disp( i != m_args.size() - 1 || m_assn_args.size() > 0 );
	}
	for( size_t i = 0; i < m_assn_args.size(); ++i ) {
		m_assn_args[ i ]->disp( i != m_assn_args.size() - 1 );
	}
	io::trem();
}
const std::vector< const stmt_base_t * > & stmt_fn_call_args_t::args() const { return m_args; }
const std::vector< const stmt_base_t * > & stmt_fn_call_args_t::assn_args() const { return m_assn_args; }
