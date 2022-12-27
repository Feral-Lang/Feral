#pragma once

#include "Core.hpp"

namespace fer
{

class ModuleLoc;

enum class Opcode : uint8_t
{
	LOAD_DATA,     // laod a const int/float/char/string from operand on the stack
	UNLOAD,	       // unload from stack; operand = count of unloads to perform
	STORE,	       // store data in a variable; no operand
	CREATE,	       // create a variable with name as operand, value present in stack
	CREATE_IN,     // create a variable with name as operand, value and 'in' present in stack
	PUSH_BLOCK,    // push a layer for variables on stack; operand = count of layers to push
	POP_BLOCK,     // pop a layer of variables from stack; operand = count of layers to pop
	PUSH_LOOP,     // special handling for loops
	POP_LOOP,      // special handling for loops
	RETURN,	       // self explanatory; operand = true if a val exists, false for void/nil
	BLOCK_TILL,    // create a block (for function); operand = index till which block exists
	CREATE_FN,     // self explanatory; operand = string:
		       // first char '1' if function contains keyword arg, else '0';
		       // second char '1' if function contains variadic, else '0';
		       // rest chars '1' if the equivalent arg has default value, else '0'
	CONTINUE,      // self explanatory; no operand
	BREAK,	       // self explanatory, no operand
	JMP,	       // jump unconditionally; operand = index in bytecode to jump to
	JMP_TRUE,      // jump if true; operand = index in bytecode to jump to
	JMP_FALSE,     // jump if false; operand = index in bytecode to jump to
	JMP_TRUE_POP,  // jump if true; operand = index in bytecode to jump to
	JMP_FALSE_POP, // jump if false; operand = index in bytecode to jump to

	// for 'or' block
	PUSH_JMP,      // marks the position to jump to if 'or' exists in an expression
	PUSH_JMP_NAME, // sets the variable name for last jump instr (must occur after PUSH_JMP)
	POP_JMP,       // unmarks the position to jump to if 'or' exists in an expression

	ATTR, // operand = string - attribute name

	// arginfo:
	// 0 => simple
	// 1 => keyword
	// 2 => unpack
	CALL,	  // operand = string of arginfo
	MEM_CALL, // operand = string of arginfo

	LAST, // used only as a case in execution
};

StringRef getOpcodeStr(Opcode opcode);

union Data
{
	StringRef s;
	int64_t i;
	long double d;
	char c;
	bool b;
};

enum class DataType : uint8_t
{
	BOOL,
	NIL,
	INT,
	FLT,
	CHR,
	STR,
	IDEN,
};

class Instruction
{
	Data data;
	const ModuleLoc *loc;
	DataType dtype;
	Opcode opcode;

public:
	Instruction(Opcode opcode, const ModuleLoc *loc, StringRef data, DataType dtype);
	Instruction(Opcode opcode, const ModuleLoc *loc, int64_t data);
	Instruction(Opcode opcode, const ModuleLoc *loc, long double data);
	Instruction(Opcode opcode, const ModuleLoc *loc, char data);
	Instruction(Opcode opcode, const ModuleLoc *loc, bool data);
	Instruction(Opcode opcode, const ModuleLoc *loc); // for nil

#define isDataX(X, ENUMVAL) \
	inline bool isData##X() const { return dtype == DataType::ENUMVAL; }
	isDataX(Bool, BOOL);
	isDataX(Nil, NIL);
	isDataX(Chr, CHR);
	isDataX(Int, INT);
	isDataX(Flt, FLT);
	isDataX(Str, STR);
	isDataX(Iden, IDEN);

	inline void setInt(int64_t dat) { data.i = dat; }

	inline const ModuleLoc *getLoc() const { return loc; }
	inline StringRef getDataStr() const { return data.s; }
	inline int64_t getDataInt() const { return data.i; }
	inline long double getDataFlt() const { return data.d; }
	inline char getDataChr() const { return data.c; }
	inline bool getDataBool() const { return data.b; }

	inline Data &getData() { return data; }
	inline DataType getDataType() const { return dtype; }
	inline Opcode getOpcode() const { return opcode; }
};

class Bytecode
{
	Vector<Instruction> code;

public:
	Bytecode();
	~Bytecode();

	inline void addInstrStr(Opcode opcode, const ModuleLoc *loc, StringRef data)
	{
		code.emplace_back(opcode, loc, data, DataType::STR);
	}
	inline void addInstrIden(Opcode opcode, const ModuleLoc *loc, StringRef data)
	{
		code.emplace_back(opcode, loc, data, DataType::IDEN);
	}
	inline void addInstrInt(Opcode opcode, const ModuleLoc *loc, int64_t data)
	{
		code.emplace_back(opcode, loc, data);
	}
	inline void addInstrFlt(Opcode opcode, const ModuleLoc *loc, long double data)
	{
		code.emplace_back(opcode, loc, data);
	}
	inline void addInstrChr(Opcode opcode, const ModuleLoc *loc, char data)
	{
		code.emplace_back(opcode, loc, data);
	}
	inline void addInstrBool(Opcode opcode, const ModuleLoc *loc, bool data)
	{
		code.emplace_back(opcode, loc, data);
	}
	inline void addInstrNil(Opcode opcode, const ModuleLoc *loc)
	{
		code.emplace_back(opcode, loc);
	}

	inline void updateInstrInt(size_t instr_idx, int64_t data) { code[instr_idx].setInt(data); }

	inline void pop() { code.pop_back(); }
	inline void erase(size_t idx) { code.erase(code.begin() + idx); }
	inline size_t getLastIndex() const { return code.size() - 1; }
	inline Instruction &getInstrAt(size_t idx) { return code[idx]; }
	inline size_t size() const { return code.size(); }
	inline Vector<Instruction> &getBytecode() { return code; }

	void dump(OStream &os) const;
};

} // namespace fer