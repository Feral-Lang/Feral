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
	OP_STORE,	// store in a name, from stack
	OP_GSTORE,	// store in a name (globally), from stack
	OP_LOAD,	// load from operand, onto stack
	OP_ULOAD,	// unload (pop) from stack

	OP_JMP,		// unconditional jump to index
	OP_JMPT,	// jump to index if top element on stack is true
	OP_JMPF,	// jump to index if top element on stack is false
	OP_JMPTNU,	// jump to index if top element on stack is true - but don't unload it
	OP_JMPFNU,	// jump to index if top element on stack is false - but don't unload it

	OP_MKFN,	// create a function object

	OP_BLKA,	// add count scopes
	OP_BLKR,	// rem count scopes

	OP_FNCL,	// call a function
	OP_ATTR,	// get attribute from an object

	OP_BLKT,	// block till
	OP_ARGT,	// args till

	OP_RET,		// return data

	_OP_LAST,
};

extern const char * OpCodeStrs[ _OP_LAST ];

enum OpDataType
{
	ODT_U,
	ODT_F,
	ODT_S,
	ODT_I,
	ODT_B,
};

union op_data_t
{
	size_t u;
	double f;
	char * s;
	int i;
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
	void addu( const size_t & idx, const OpCodes op, const size_t & data );
	void addf( const size_t & idx, const OpCodes op, const double & data );
	void adds( const size_t & idx, const OpCodes op, const std::string & data );
	void addi( const size_t & idx, const OpCodes op, const int & data );
	void addb( const size_t & idx, const OpCodes op, const bool & data );

	const std::vector< op_t > & bcode() const;
	size_t size() const;
};

#endif // VM_OPCODES_HPP
