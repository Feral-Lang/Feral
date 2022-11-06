#include "Bytecode.hpp"

#include "Module.hpp"

namespace fer
{

StringRef getOpcodeStr(Opcode opcode)
{
	switch(opcode) {
	case Opcode::LOAD_CONST: return "LOAD_CONST";
	case Opcode::UNLOAD: return "UNLOAD";
	case Opcode::CREATE_VAR: return "CREATE_VAR";
	case Opcode::CREATE_CONST: return "CREATE_CONST";
	case Opcode::PUSH_LAYER: return "PUSH_LAYER";
	case Opcode::POP_LAYER: return "POP_LAYER";
	default: break;
	}
	return "";
}

Instruction::Instruction(Opcode opcode, const ModuleLoc *loc, StringRef data)
	: data{.s = data}, loc(loc), dtype(DataType::STR), opcode(opcode)
{}
Instruction::Instruction(Opcode opcode, const ModuleLoc *loc, int64_t data)
	: data{.i = data}, loc(loc), dtype(DataType::INT), opcode(opcode)
{}
Instruction::Instruction(Opcode opcode, const ModuleLoc *loc, long double data)
	: data{.d = data}, loc(loc), dtype(DataType::FLT), opcode(opcode)
{}
Instruction::Instruction(Opcode opcode, const ModuleLoc *loc, char data)
	: data{.c = data}, loc(loc), dtype(DataType::CHR), opcode(opcode)
{}

Bytecode::Bytecode() {}
Bytecode::~Bytecode() {}

void Bytecode::dump(OStream &os) const
{
	for(size_t idx = 0; idx < code.size(); ++idx) {
		auto &i = code[idx];
		os << idx << "\t" << getOpcodeStr(i.getOpcode()) << "\t";
		if(i.isInt()) os << "[int] " << i.getDataInt() << "\n";
		if(i.isFlt()) os << "[flt] " << i.getDataFlt() << "\n";
		if(i.isChr()) os << "[chr] " << i.getDataChr() << "\n";
		if(i.isStr()) os << "[str] " << i.getDataStr() << "\n";
	}
}

} // namespace fer