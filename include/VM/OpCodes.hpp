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
	OP_JMPT,	// jump to index if top element on stack is true - will not unload if true
	OP_JMPF,	// jump to index if top element on stack is false - will not unload if false
	OP_JMPTPOP,	// jump to index if top element on stack is true - will pop unconditionally
	OP_JMPFPOP,	// jump to index if top element on stack is false - will pop unconditionally
	OP_JMPN,	// jump to index if top element on stack is nil (won't pop otherwise)

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

	// for loops
	OP_PUSH_LOOP,	// marks a loop's beginning for variable stack
	OP_POP_LOOP,	// marks a loop's ending for variable stack

	// for 'or' keyword
	OP_PUSH_JMP,	// marks the position to jump to if 'or' exists in an expression
	OP_POP_JMP,	// unmarks the position to jump to if 'or' exists in an expression

	_OP_LAST,
};

extern const char * OpCodeStrs[ _OP_LAST ];

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
	// src_id is set afterwards since srcfile_t is usually made at later time
	size_t src_id;
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

	inline const std::vector< op_t > & get() const { return m_bcode; }
	inline std::vector< op_t > & getmut() { return m_bcode; }
	inline size_t size() const { return m_bcode.size(); }
};

#endif // VM_OPCODES_HPP
