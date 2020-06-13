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

bool stmt_single_operand_stmt_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	size_t blk_till_pos = bc.size();

	if( m_operand ) m_operand->gen_code( bc );

	if( m_sost->type == TOK_RETURN ) {
		bc.addb( idx(), OP_RET, m_operand );
	} else if( m_sost->type == TOK_CONTINUE ) {
		// placeholder (updated in For, Foreach, While)
		bc.addsz( idx(), OP_CONTINUE, 0 );
	} else if( m_sost->type == TOK_BREAK ) {
		// placeholder (updated in For, Foreach, While)
		bc.addsz( idx(), OP_BREAK, 0 );
	}
	return true;
}
