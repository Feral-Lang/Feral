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

bool stmt_block_t::gen_code( bcode_t & bc ) const
{
	if( !m_no_brace ) bc.addsz( idx(), OP_BLKA, 1 );

	for( auto & stmt : m_stmts ) {
		if( !stmt->gen_code( bc ) ) return false;
	}

	if( !m_no_brace ) bc.addsz( idx(), OP_BLKR, 1 );
	return true;
}
