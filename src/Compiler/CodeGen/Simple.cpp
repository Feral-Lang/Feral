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

#include "Compiler/CodeGen/Internal.hpp"

bool stmt_simple_t::gen_code( bcode_t & bc ) const
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
