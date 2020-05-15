/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Compiler/CodeGen/Internal.hpp"

bool stmt_block_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	if( !f1 ) bc.addsz( idx(), OP_BLKA, 1 );

	for( auto & stmt : m_stmts ) {
		if( !stmt->gen_code( bc, false, stmt->type() == GT_EXPR ) ) return false;
	}

	if( !f1 ) bc.addsz( idx(), OP_BLKR, 1 );
	return true;
}
