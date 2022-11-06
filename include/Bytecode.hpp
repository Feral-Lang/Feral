#pragma once

#include "Core.hpp"

namespace fer
{

class ModuleLoc;

enum class Opcode : uint8_t
{
	LOAD_CONST,   // laod a const int/float/char/string from operand on the stack
	UNLOAD,	      // unload from stack; operand = count of unloads to perform
	CREATE_VAR,   // create a variable with name as operand, value present in stack
	CREATE_CONST, // create a const variable with name as operand, value present in stack
	PUSH_LAYER,   // push a layer for variables on stack; operand = count of layers to push
	POP_LAYER,    // pop a layer of variables from stack; operand = count of layers to pop

	_LAST,
};

StringRef getOpcodeStr(Opcode opcode);

union Data
{
	StringRef s;
	int64_t i;
	long double d;
	char c;
};

enum class DataType : uint8_t
{
	INT,
	FLT,
	CHR,
	STR,
};

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

#define isDataX(X, ENUMVAL) \
	inline bool is##X() const { return dtype == DataType::ENUMVAL; }
	isDataX(Chr, CHR);
	isDataX(Int, INT);
	isDataX(Flt, FLT);
	isDataX(Str, STR);

#define isX(X, ENUMVAL) \
	inline bool is##X() const { return opcode == Opcode::ENUMVAL; }
	isX(LoadConst, LOAD_CONST);
	isX(Unload, UNLOAD);
	isX(CreateVar, CREATE_VAR);

	inline const ModuleLoc *getLoc() const { return loc; }
	inline StringRef getDataStr() const { return data.s; }
	inline int64_t getDataInt() const { return data.i; }
	inline long double getDataFlt() const { return data.d; }
	inline char getDataChr() const { return data.c; }

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

	inline void pop() { code.pop_back(); }
	inline void erase(size_t idx) { code.erase(code.begin() + idx); }
	inline size_t getLastIndex() const { return code.size() - 1; }
	inline Instruction &getInstrAt(size_t idx) { return code[idx]; }
	inline size_t size() const { return code.size(); }
	inline Vector<Instruction> &getBytecode() { return code; }

	void dump(OStream &os) const;
};

} // namespace fer