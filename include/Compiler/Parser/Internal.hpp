/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef COMPILER_PARSER_INTERNAL_HPP
#define COMPILER_PARSER_INTERNAL_HPP

#include "ParseHelper.hpp" // for all the parse function sources
#include "Stmts.hpp"

Errors parse_block( phelper_t & ph, stmt_base_t * & loc, const bool with_brace = true );

Errors parse_expr_cols( phelper_t & ph, stmt_base_t * & loc );
Errors parse_expr( phelper_t & ph, stmt_base_t * & loc );
Errors parse_expr_16( phelper_t & ph, stmt_base_t * & loc );
Errors parse_expr_15( phelper_t & ph, stmt_base_t * & loc );
Errors parse_expr_14( phelper_t & ph, stmt_base_t * & loc );
Errors parse_expr_13( phelper_t & ph, stmt_base_t * & loc );
Errors parse_expr_12( phelper_t & ph, stmt_base_t * & loc );
Errors parse_expr_11( phelper_t & ph, stmt_base_t * & loc );
Errors parse_expr_10( phelper_t & ph, stmt_base_t * & loc );
Errors parse_expr_09( phelper_t & ph, stmt_base_t * & loc );
Errors parse_expr_08( phelper_t & ph, stmt_base_t * & loc );
Errors parse_expr_07( phelper_t & ph, stmt_base_t * & loc );
Errors parse_expr_06( phelper_t & ph, stmt_base_t * & loc );
Errors parse_expr_05( phelper_t & ph, stmt_base_t * & loc );
Errors parse_expr_04( phelper_t & ph, stmt_base_t * & loc );
Errors parse_expr_03( phelper_t & ph, stmt_base_t * & loc );
Errors parse_expr_02( phelper_t & ph, stmt_base_t * & loc );
Errors parse_expr_01( phelper_t & ph, stmt_base_t * & loc );
// make_const if true, changes type of token from IDEN to STR (useful for, say, after TOK_DOTs)
Errors parse_term( phelper_t & ph, stmt_base_t * & loc, const bool make_const = false );

Errors parse_var_decl( phelper_t & ph, stmt_base_t * & loc );
Errors parse_var_decl_base( phelper_t & ph, stmt_base_t * & loc );

Errors parse_fn_decl( phelper_t & ph, stmt_base_t * & loc );
Errors parse_fn_decl_args( phelper_t & ph, stmt_base_t * & loc );

Errors parse_fn_call_args( phelper_t & ph, stmt_base_t * & loc );

Errors parse_single_operand_stmt( phelper_t & ph, stmt_base_t * & loc );

Errors parse_conditional( phelper_t & ph, stmt_base_t * & loc );

Errors parse_for( phelper_t & ph, stmt_base_t * & loc );
Errors parse_foreach( phelper_t & ph, stmt_base_t * & loc );
Errors parse_while( phelper_t & ph, stmt_base_t * & loc );

#endif // COMPILER_PARSER_INTERNAL_HPP
