/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <cstring>

#include "OpCodes.hpp"

const char * OpCodeStrs[ _OP_LAST ] = {
	"STORE",	// store in a name, from stack
	"GLOBAL_STORE",	// store in a name (globally), from stack
	"LOAD",		// load from operand, onto stack
	"UNLOAD",	// unload (pop) from stack

	"JMP",		// unconditional jump to index
	"JMP_TRUE",	// jump to index if top element on stack is true
	"JMP_FALSE",	// jump to index if top element on stack is false
	"JMP_TRUE_NU",	// jump to index if top element on stack is true - but don't unload it
	"JMP_FALSE_NU",	// jump to index if top element on stack is false - but don't unload it

	"MK_FUNC",	// create a function object

	"BLK_ADD",	// add count scopes
	"BLK_REM",	// rem count scopes

	"FUNC_CALL",	// call a function
	"ATTRIBUTE",	// get attribute from an object

	"BLK_TILL",	// block till
	"ARG_TILL",	// args till
};

inline char * scpy( const std::string & str )
{
	char * res = new char[ str.size() + 1 ];
	return strcpy( res, str.c_str() );
}

bcode_t::~bcode_t()
{
	for( auto & bc : m_bcode ) {
		if( bc.dtype == ODT_S ) delete bc.data.s;
	}
}

void bcode_t::addu( const size_t & idx, const OpCodes op, const size_t & data )
{
	m_bcode.push_back( op_t{ idx, op, ODT_U, { .u = data } } );
}
void bcode_t::addf( const size_t & idx, const OpCodes op, const double & data )
{
	m_bcode.push_back( op_t{ idx, op, ODT_F, { .f = data } } );
}
void bcode_t::adds( const size_t & idx, const OpCodes op, const std::string & data )
{
	m_bcode.push_back( op_t{ idx, op, ODT_S, { .s = scpy( data ) } } );
}
void bcode_t::addi( const size_t & idx, const OpCodes op, const int & data )
{
	m_bcode.push_back( op_t{ idx, op, ODT_I, { .i = data } } );
}
void bcode_t::addb( const size_t & idx, const OpCodes op, const bool & data )
{
	m_bcode.push_back( op_t{ idx, op, ODT_B, { .b = data } } );
}

const std::vector< op_t > & bcode_t::bcode() const { return m_bcode; }
size_t bcode_t::size() const { return m_bcode.size(); }
