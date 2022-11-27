#pragma once

#include "Core.hpp"

namespace fer
{

class ModuleLoc;

enum class Opcode : uint8_t
{
	LOAD_CONST,    // laod a const int/float/char/string from operand on the stack
	UNLOAD,	       // unload from stack; operand = count of unloads to perform
	CREATE_VAR,    // create a variable with name as operand, value present in stack
	CREATE_CONST,  // create a const variable with name as operand, value present in stack
	PUSH_LAYER,    // push a layer for variables on stack; operand = count of layers to push
	POP_LAYER,     // pop a layer of variables from stack; operand = count of layers to pop
	LOAD_ARG,      // load a funtion arg by index (to create func args in body); operand = index
		       // of arg to be fetched
	RETURN,	       // self explanatory; operand = true if a val exists, false for void
	BLOCK_TILL,    // create a block (for function); operand = index till which block exists
	CREATE_FN,     // self explanatory; operand = first char boolean -> true if variadic,
		       // second char -> number of args (char itself is the number)
		       // also loads the block from stack (created via BLOCK_TILL)
	CONTINUE,      // self explanatory; no operand
	BREAK,	       // self explanatory, no operand
	JMP,	       // jump unconditionally; operand = index in bytecode to jump to
	JMP_TRUE,      // jump if true; operand = index in bytecode to jump to
	JMP_FALSE,     // jump if false; operand = index in bytecode to jump to
	JMP_POP,       // jump unconditionally; operand = index in bytecode to jump to
	JMP_TRUE_POP,  // jump if true; operand = index in bytecode to jump to
	JMP_FALSE_POP, // jump if false; operand = index in bytecode to jump to

	// Operators
	// None of the operators have an operand
	ASSN,
	// Arithmetic
	ADD,
	SUB,
	MUL,
	DIV,
	MOD,
	ADD_ASSN,
	SUB_ASSN,
	MUL_ASSN,
	DIV_ASSN,
	MOD_ASSN,
	// Post/Pre Inc/Dec
	XINC,
	INCX,
	XDEC,
	DECX,
	// Unary
	UADD,
	USUB,
	UAND, // address of
	UMUL, // dereference
	// Logic (LAND and LOR are handled using jmps)
	LNOT,
	// Comparison
	EQ,
	LT,
	GT,
	LE,
	GE,
	NE,
	// Bitwise
	BAND,
	BOR,
	BNOT,
	BXOR,
	BAND_ASSN,
	BOR_ASSN,
	BNOT_ASSN,
	BXOR_ASSN,
	// Others
	LSHIFT,
	RSHIFT,
	LSHIFT_ASSN,
	RSHIFT_ASSN,
	SUBS,
	DOT,	// attribute operation
	FNCALL, // operand = count of args
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
};

inline int64_t addVariadicFlag(int64_t arginfo) { return arginfo |= (int64_t)1 << 63; }
inline bool hasVariadicFlag(int64_t arginfo) { return arginfo & ((int64_t)1 << 63); }

class Instruction
{
	Data data;
	const ModuleLoc *loc;
	DataType dtype;
	Opcode opcode;

public:
	Instruction(Opcode opcode, const ModuleLoc *loc, StringRef data);
	Instruction(Opcode opcode, const ModuleLoc *loc, int64_t data);
	Instruction(Opcode opcode, const ModuleLoc *loc, long double data);
	Instruction(Opcode opcode, const ModuleLoc *loc, char data);
	Instruction(Opcode opcode, const ModuleLoc *loc, bool data);
	Instruction(Opcode opcode, const ModuleLoc *loc); // for nil

#define isDataX(X, ENUMVAL) \
	inline bool is##X() const { return dtype == DataType::ENUMVAL; }
	isDataX(Bool, BOOL);
	isDataX(Nil, NIL);
	isDataX(Chr, CHR);
	isDataX(Int, INT);
	isDataX(Flt, FLT);
	isDataX(Str, STR);

#define isX(X, ENUMVAL) \
	inline bool is##X() const { return opcode == Opcode::ENUMVAL; }
	isX(LoadConst, LOAD_CONST);
	isX(Unload, UNLOAD);
	isX(CreateVar, CREATE_VAR);

	inline void setInt(int64_t dat) { data.i = dat; }

	inline const ModuleLoc *getLoc() const { return loc; }
	inline StringRef getDataStr() const { return data.s; }
	inline int64_t getDataInt() const { return data.i; }
	inline long double getDataFlt() const { return data.d; }
	inline char getDataChr() const { return data.c; }
	inline bool getDataBool() const { return data.b; }

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
		code.emplace_back(opcode, loc, data);
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