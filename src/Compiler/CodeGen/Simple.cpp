/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Internal.hpp"

bool stmt_simple_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	if( !m_val ) return true;

	if( m_val->type == TOK_INT ) bc.adds( m_val->pos, OP_LOAD, ODT_INT, m_val->data );
	else if( m_val->type == TOK_FLT ) bc.adds( m_val->pos, OP_LOAD, ODT_FLT, m_val->data );
	else if( m_val->type == TOK_STR ) bc.adds( m_val->pos, OP_LOAD, ODT_STR, m_val->data );
	else if( m_val->type == TOK_IDEN ) bc.adds( m_val->pos, OP_LOAD, ODT_IDEN, m_val->data );
	else if( m_val->type == TOK_TRUE || m_val->type == TOK_FALSE ) bc.addb( m_val->pos, OP_LOAD, m_val->type == TOK_TRUE );
	else if( m_val->type == TOK_NIL ) bc.add( m_val->pos, OP_LOAD );

	return true;
}
