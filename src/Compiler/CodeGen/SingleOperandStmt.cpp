/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Internal.hpp"

bool stmt_single_operand_stmt_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	size_t blk_till_pos = bc.size();
	if( m_sost->type == TOK_DEFER ) {
		bc.addsz( idx(), OP_BODY_TILL, 0 );
	}
	if( m_operand ) m_operand->gen_code( bc );

	if( m_sost->type == TOK_RETURN ) {
		bc.addb( idx(), OP_RET, m_operand );
	} else if( m_sost->type == TOK_CONTINUE ) {
		bc.add( idx(), OP_CONTINUE );
	} else if( m_sost->type == TOK_BREAK ) {
		bc.add( idx(), OP_BREAK );
	} else if( m_sost->type == TOK_DEFER ) {
		bc.updatesz( blk_till_pos, bc.size() );
		bc.add( idx(), OP_DEFER );
	}
	return true;
}
