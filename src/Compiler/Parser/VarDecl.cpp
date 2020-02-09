/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Internal.hpp"

Errors parse_var_decl( phelper_t & ph, stmt_base_t * & loc, const bool local_only )
{
	std::vector< const stmt_var_decl_base_t * > decls;
	stmt_base_t * decl = nullptr;
	VarDeclType dtype = VDT_LOCAL;

	size_t idx = ph.peak()->pos;
	if( !ph.accept( TOK_LET ) && ( local_only || !ph.accept( TOK_GLOBAL ) ) ) {
		ph.fail( "expected keyword %s here, but found: '%s'",
			 local_only ? "'let'" : "'global' or 'let'",
			 TokStrs[ ph.peakt() ] );
		goto fail;
	}
	if( ph.peakt() == TOK_GLOBAL ) dtype = VDT_GLOBAL;
	ph.next();

begin:
	if( parse_var_decl_base( ph, decl ) != E_OK ) {
		goto fail;
	}
	decls.push_back( ( stmt_var_decl_base_t * )decl );
	decl = nullptr;

	if( ph.accept( TOK_COMMA ) ) {
		ph.next();
		goto begin;
	}

	if( !ph.accept( TOK_COLS ) ) {
		ph.fail( "expected semicolon after variable declaration, found: '%s'",
			 TokStrs[ ph.peakt() ] );
		goto fail;
	}
	ph.next();
	loc = new stmt_var_decl_t( dtype, decls, idx );
	return E_OK;
fail:
	for( auto & decl : decls ) delete decl;
	return E_PARSE_FAIL;
}

Errors parse_var_decl_base( phelper_t & ph, stmt_base_t * & loc )
{
	stmt_base_t * lhs = nullptr, * rhs = nullptr;

	// iden index
	size_t idx = ph.peak()->pos;

	// 01 = parenthesized expression, func call, subscript, dot
	if( parse_expr_01( ph, lhs ) != E_OK ) {
		goto fail;
	}

	if( !ph.accept( TOK_ASSN ) ) {
		ph.fail( "expected assignment operator here for var decl, but found: '%s'",
			 TokStrs[ ph.peakt() ] );
		goto fail;
	}
	ph.next();

	// 13 = everything excluding comma and assignments (=, +=, ...)
	if( parse_expr_13( ph, rhs ) != E_OK ) {
		goto fail;
	}

	loc = new stmt_var_decl_base_t( lhs, rhs );
	return E_OK;
fail:
	if( lhs ) delete lhs;
	if( rhs ) delete rhs;
	return E_PARSE_FAIL;
}