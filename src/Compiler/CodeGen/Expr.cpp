/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Internal.hpp"

// used for AND and OR operations - all locations from where to jump
static std::vector< size_t > jmp_locs;

// f1 is used to denote top level in a nested expression (false = it is top level)
bool stmt_expr_t::gen_code( bcode_t & bc, const bool f1, const bool f2 ) const
{
	size_t before_jmp_locs_count = jmp_locs.size();

	m_lhs->gen_code( bc, m_lhs->type() == GT_EXPR );

	if( !m_oper ) return true;

	if( m_oper->type == TOK_AND || m_oper->type == TOK_OR ) {
		jmp_locs.push_back( bc.size() );
		// here, 0 is a placeholder
		if( !f1 ) {
			bc.addsz( m_oper->pos, m_oper->type == TOK_AND ? OP_JMPF : OP_JMPT, 0 );
		} else {
			bc.addsz( m_oper->pos, m_oper->type == TOK_AND ? OP_JMPFNU : OP_JMPTNU, 0 );
		}
	}

	if( m_rhs ) {
		m_rhs->gen_code( bc, m_rhs->type() == GT_EXPR );
	}

	if( m_oper->type == TOK_AND || m_oper->type == TOK_OR ) {
		size_t curr_sz = bc.size();
		for( size_t i = before_jmp_locs_count; i < jmp_locs.size(); ++i ) {
			bc.updatesz( jmp_locs[ i ], curr_sz );
		}
		size_t jmps_to_rem = jmp_locs.size() - before_jmp_locs_count;
		for( size_t i = 0; i < jmps_to_rem; ++i ) jmp_locs.pop_back();
		return true;
	}

	// all operators
	if( m_oper->type == TOK_ASSN ) bc.add( m_oper->pos, OP_STORE );

	else if( m_oper->type == TOK_ADD ) bc.addsz( m_oper->pos, OP_BINARY, OPB_ADD );
	else if( m_oper->type == TOK_SUB ) bc.addsz( m_oper->pos, OP_BINARY, OPB_SUB );
	else if( m_oper->type == TOK_MUL ) bc.addsz( m_oper->pos, OP_BINARY, OPB_MUL );
	else if( m_oper->type == TOK_DIV ) bc.addsz( m_oper->pos, OP_BINARY, OPB_DIV );
	else if( m_oper->type == TOK_MOD ) bc.addsz( m_oper->pos, OP_BINARY, OPB_MOD );

	else if( m_oper->type == TOK_ADD_ASSN ) bc.addsz( m_oper->pos, OP_BINARY, OPB_ADD_ASSN );
	else if( m_oper->type == TOK_SUB_ASSN ) bc.addsz( m_oper->pos, OP_BINARY, OPB_SUB_ASSN );
	else if( m_oper->type == TOK_MUL_ASSN ) bc.addsz( m_oper->pos, OP_BINARY, OPB_MUL_ASSN );
	else if( m_oper->type == TOK_DIV_ASSN ) bc.addsz( m_oper->pos, OP_BINARY, OPB_DIV_ASSN );
	else if( m_oper->type == TOK_MOD_ASSN ) bc.addsz( m_oper->pos, OP_BINARY, OPB_MOD_ASSN );

	else if( m_oper->type == TOK_POW ) bc.addsz( m_oper->pos, OP_BINARY, OPB_POW );

	else if( m_oper->type == TOK_NOT ) bc.addsz( m_oper->pos, OP_UNARY, OPU_NOT );

	else if( m_oper->type == TOK_BAND ) bc.addsz( m_oper->pos, OP_BINARY, OPB_BAND );
	else if( m_oper->type == TOK_BOR )  bc.addsz( m_oper->pos, OP_BINARY, OPB_BOR );
	else if( m_oper->type == TOK_BNOT ) bc.addsz( m_oper->pos, OP_BINARY, OPB_BNOT );
	else if( m_oper->type == TOK_BXOR ) bc.addsz( m_oper->pos, OP_BINARY, OPB_BXOR );

	else if( m_oper->type == TOK_BAND_ASSN ) bc.addsz( m_oper->pos, OP_BINARY, OPB_BAND_ASSN );
	else if( m_oper->type == TOK_BOR_ASSN )  bc.addsz( m_oper->pos, OP_BINARY, OPB_BOR_ASSN );
	else if( m_oper->type == TOK_BNOT_ASSN ) bc.addsz( m_oper->pos, OP_BINARY, OPB_BNOT_ASSN );
	else if( m_oper->type == TOK_BXOR_ASSN ) bc.addsz( m_oper->pos, OP_BINARY, OPB_BXOR_ASSN );

	else if( m_oper->type == TOK_LSHIFT ) bc.addsz( m_oper->pos, OP_BINARY, OPB_LSHIFT );
	else if( m_oper->type == TOK_RSHIFT ) bc.addsz( m_oper->pos, OP_BINARY, OPB_RSHIFT );

	else if( m_oper->type == TOK_LSHIFT_ASSN ) bc.addsz( m_oper->pos, OP_BINARY, OPB_LSHIFT_ASSN );
	else if( m_oper->type == TOK_RSHIFT_ASSN ) bc.addsz( m_oper->pos, OP_BINARY, OPB_RSHIFT_ASSN );

	else if( m_oper->type == TOK_XINC ) bc.addsz( m_oper->pos, OP_UNARY, OPU_XINC );
	else if( m_oper->type == TOK_XDEC ) bc.addsz( m_oper->pos, OP_UNARY, OPU_XDEC );
	else if( m_oper->type == TOK_INCX ) bc.addsz( m_oper->pos, OP_UNARY, OPU_INCX );
	else if( m_oper->type == TOK_DECX ) bc.addsz( m_oper->pos, OP_UNARY, OPU_DECX );

	else if( m_oper->type == TOK_UADD ) bc.addsz( m_oper->pos, OP_UNARY, OPU_ADD );
	else if( m_oper->type == TOK_USUB ) bc.addsz( m_oper->pos, OP_UNARY, OPU_SUB );

	else if( m_oper->type == TOK_EQ ) bc.addsz( m_oper->pos, OP_COMP, OPC_EQ );
	else if( m_oper->type == TOK_LT ) bc.addsz( m_oper->pos, OP_COMP, OPC_LT );
	else if( m_oper->type == TOK_GT ) bc.addsz( m_oper->pos, OP_COMP, OPC_GT );
	else if( m_oper->type == TOK_LE ) bc.addsz( m_oper->pos, OP_COMP, OPC_LE );
	else if( m_oper->type == TOK_GE ) bc.addsz( m_oper->pos, OP_COMP, OPC_GE );
	else if( m_oper->type == TOK_NE ) bc.addsz( m_oper->pos, OP_COMP, OPC_NE );

	else if( m_oper->type == TOK_DOT ) bc.add( m_oper->pos, OP_ATTR );

	else if( m_oper->type == TOK_OPER_FN ) bc.addb( m_oper->pos, OP_FNCL, m_rhs );
	else if( m_oper->type == TOK_OPER_SUBS ) bc.addsz( m_oper->pos, OP_BINARY, OPB_SUBSCR );

	return true;
}
