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
	"CREATE",	// create a new variable
	"STORE",	// store in a name, from stack
	"LOAD",		// load from operand, onto stack
	"UNLOAD",	// unload (pop) from stack

	"JMP",		// unconditional jump to index
	"JMP_NIL",	// jump to index if top element on stack is nil (won't pop otherwise)
	"JMP_TRUE",	// jump to index if top element on stack is true
	"JMP_FALSE",	// jump to index if top element on stack is false
	"JMP_TRUE_NU",	// jump to index if top element on stack is true - but don't unload it
	"JMP_FALSE_NU",	// jump to index if top element on stack is false - but don't unload it

	"BODY_TILL",	// jump to index which is where the body (of a function) ends + 1
	"MK_FUNC",	// create a function object

	"BLK_ADD",	// add count scopes
	"BLK_REM",	// rem count scopes

	"FUNC_CALL",	// call a function
	"ATTRIBUTE",	// get attribute from an object

	"BLK_TILL",	// block till
	"ARG_TILL",	// args till

	"RETURN",	// return data
	"CONTINUE",	// size_t operand - jump to
	"BREAK",	// size_t operand - jump to
	"DEFER",	// can take expression or block - bool - true takes expr, false takes block

	// operators
	"BINARY",
	"UNARY",
	"COMPARISON",
};

const char * OpBinaryStrs[ _OPB_LAST ] = {
	"+",
	"-",
	"*",
	"/",
	"%",

	"+=",
	"-=",
	"*=",
	"/=",
	"%=",

	"**",

	"&",
	"|",
	"~",
	"^",

	"&="
	"|=",
	"~=",
	"^=",

	"<<",
	">>",

	"<<=",
	">>=",

	"[]",
};

const char * OpUnaryStrs[ _OPU_LAST ] = {
	"X++",
	"++X",
	"X--",
	"--X",

	"!",

	"U+",
	"U-",
};

const char * OpCompStrs[ _OPC_LAST ] = {
	"==",
	"<",
	">",
	"<=",
	">=",
	"!=",
};

const char * OpDataTypeStrs[ _ODT_LAST ] = {
	"INT",
	"FLT",
	"STR",
	"IDEN",

	"SZ",

	"BOOL",

	"NIL",
};

inline char * scpy( const std::string & str )
{
	char * res = new char[ str.size() + 1 ];
	return strcpy( res, str.c_str() );
}

bcode_t::~bcode_t()
{
	for( auto & op : m_bcode ) {
		if( op.dtype != ODT_SZ && op.dtype != ODT_BOOL ) delete[] op.data.s;
	}
}

void bcode_t::add( const size_t & idx, const OpCodes op )
{
	m_bcode.push_back( op_t{ idx, op, ODT_NIL, { .s = nullptr } } );
}
void bcode_t::adds( const size_t & idx, const OpCodes op, const OpDataType dtype, const std::string & data )
{
	m_bcode.push_back( op_t{ idx, op, dtype, { .s = scpy( data ) } } );
}
void bcode_t::addb( const size_t & idx, const OpCodes op, const bool & data )
{
	m_bcode.push_back( op_t{ idx, op, ODT_BOOL, { .b = data } } );
}
void bcode_t::addsz( const size_t & idx, const OpCodes op, const std::string & data )
{
	m_bcode.push_back( op_t{ idx, op, ODT_SZ, { .sz = std::stoull( data ) } } );
}
void bcode_t::addsz( const size_t & idx, const OpCodes op, const size_t & data )
{
	m_bcode.push_back( op_t{ idx, op, ODT_SZ, { .sz = data } } );
}

OpCodes bcode_t::at( const size_t & pos ) const
{
	if( pos >= m_bcode.size() ) return _OP_LAST;
	return m_bcode[ pos ].op;
}

void bcode_t::updatesz( const size_t & pos, const size_t & value )
{
	if( pos >= m_bcode.size() ) return;
	m_bcode[ pos ].data.sz = value;
}

const std::vector< op_t > & bcode_t::bcode() const { return m_bcode; }
size_t bcode_t::size() const { return m_bcode.size(); }
