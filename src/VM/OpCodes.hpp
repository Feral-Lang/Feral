/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef VM_OPCODES_HPP
#define VM_OPCODES_HPP

#include <cstdlib>
#include <vector>
#include <string>

enum OpCodes
{
	OP_CREATE,	// create a new variable - bool operand - if true, it contains 'in' part (x in y = z)
	OP_STORE,	// store in a name: value from stack
	OP_LOAD,	// load from operand, onto stack
	OP_ULOAD,	// unload (pop) from stack

	OP_JMP,		// unconditional jump to index
	OP_JMPT,	// jump to index if top element on stack is true
	OP_JMPF,	// jump to index if top element on stack is false
	OP_JMPN,	// jump to index if top element on stack is nil (won't pop otherwise)
	OP_JMPTNU,	// jump to index if top element on stack is true - but don't unload it
	OP_JMPFNU,	// jump to index if top element on stack is false - but don't unload it

	OP_BODY_TILL,	// jump to index which is where the body (of a function) ends + 1
	OP_MKFN,	// create a function object

	OP_BLKA,	// add count scopes
	OP_BLKR,	// rem count scopes

	OP_FNCL,	// call a function (string arg - argument format)
	OP_MEM_FNCL,	// call a member function (string arg - argument format)
	OP_ATTR,	// get attribute from an object (operand is attribute name)

	OP_RET,		// return - bool - false pushes nil on top of stack
	OP_CONTINUE,	// size_t operand - jump to
	OP_BREAK,	// size_t operand - jump to
	OP_DEFER,	// can take expression or block - bool - true takes expr, false takes block

	// operators
	OP_BINARY,
	OP_UNARY,
	OP_COMP, // comparison

	_OP_LAST,
};

extern const char * OpCodeStrs[ _OP_LAST ];

enum OpBinary
{
	OPB_ADD,
	OPB_SUB,
	OPB_MUL,
	OPB_DIV,
	OPB_MOD,

	OPB_ADD_ASSN,
	OPB_SUB_ASSN,
	OPB_MUL_ASSN,
	OPB_DIV_ASSN,
	OPB_MOD_ASSN,

	OPB_POW,

	OPB_BAND,
	OPB_BOR,
	OPB_BNOT,
	OPB_BXOR,

	OPB_BAND_ASSN,
	OPB_BOR_ASSN,
	OPB_BNOT_ASSN,
	OPB_BXOR_ASSN,

	OPB_LSHIFT,
	OPB_RSHIFT,

	OPB_LSHIFT_ASSN,
	OPB_RSHIFT_ASSN,

	OPB_SUBSCR,

	_OPB_LAST,
};

extern const char * OpBinaryStrs[ _OPB_LAST ];

enum OpUnary {
	OPU_XINC,
	OPU_INCX,
	OPU_XDEC,
	OPU_DECX,

	OPU_NOT,

	OPU_ADD,
	OPU_SUB,

	_OPU_LAST,
};

extern const char * OpUnaryStrs[ _OPU_LAST ];

enum OpComp {
	OPC_EQ,
	OPC_LT,
	OPC_GT,
	OPC_LE,
	OPC_GE,
	OPC_NE,

	_OPC_LAST,
};

extern const char * OpCompStrs[ _OPC_LAST ];

enum OpDataType
{
	ODT_INT,
	ODT_FLT,
	ODT_STR,
	ODT_IDEN,

	ODT_SZ,

	ODT_BOOL,

	ODT_NIL,

	_ODT_LAST,
};

extern const char * OpDataTypeStrs[ _ODT_LAST ];

union op_data_t
{
	size_t sz;
	char * s;
	bool b;
};

struct op_t
{
	size_t idx;
	OpCodes op;
	OpDataType dtype;
	op_data_t data;
};

class bcode_t
{
	std::vector< op_t > m_bcode;
public:
	~bcode_t();

	void add( const size_t & idx, const OpCodes op );
	void adds( const size_t & idx, const OpCodes op, const OpDataType dtype, const std::string & data );
	void addb( const size_t & idx, const OpCodes op, const bool & data );
	void addsz( const size_t & idx, const OpCodes op, const std::string & data );
	void addsz( const size_t & idx, const OpCodes op, const size_t & data );

	OpCodes at( const size_t & pos ) const;
	void updatesz( const size_t & pos, const size_t & value );

	const std::vector< op_t > & bcode() const;
	size_t size() const;
};

#endif // VM_OPCODES_HPP
