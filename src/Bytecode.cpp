#include "Bytecode.hpp"

#include <iomanip>

#include "Module.hpp"

namespace fer
{

StringRef getOpcodeStr(Opcode opcode)
{
	switch(opcode) {
	case Opcode::LOAD_DATA: return "LOAD_DATA";
	case Opcode::UNLOAD: return "UNLOAD";
	case Opcode::STORE: return "STORE";
	case Opcode::CREATE: return "CREATE_VAR";
	case Opcode::CREATE_IN: return "CREATE_VARIN";
	case Opcode::PUSH_BLOCK: return "PUSH_BLOCK";
	case Opcode::POP_BLOCK: return "POP_BLOCK";
	case Opcode::PUSH_LOOP: return "PUSH_LOOP";
	case Opcode::POP_LOOP: return "POP_LOOP";
	case Opcode::RETURN: return "RETURN";
	case Opcode::BLOCK_TILL: return "BLOCK_TILL";
	case Opcode::CREATE_FN: return "CREATE_FN";
	case Opcode::CONTINUE: return "CONTINUE";
	case Opcode::BREAK: return "BREAK";
	case Opcode::JMP: return "JMP";
	case Opcode::JMP_TRUE: return "JMP_TRUE";
	case Opcode::JMP_FALSE: return "JMP_FALSE";
	case Opcode::JMP_TRUE_POP: return "JMP_TRUE_POP";
	case Opcode::JMP_FALSE_POP: return "JMP_FALSE_POP";
	case Opcode::PUSH_JMP: return "PUSH_JMP";
	case Opcode::PUSH_JMP_NAME: return "PUSH_JMP_NAME";
	case Opcode::POP_JMP: return "POP_JMP";
	case Opcode::ATTR: return "ATTR";
	case Opcode::CALL: return "FNCALL";
	case Opcode::MEM_CALL: return "MEM_FNCALL";
	default: break;
	}
	return "";
}

Instruction::Instruction(Opcode opcode, const ModuleLoc *loc, StringRef data, DataType dtype)
	: data{.s = data}, loc(loc), dtype(dtype), opcode(opcode)
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
Instruction::Instruction(Opcode opcode, const ModuleLoc *loc, bool data)
	: data{.b = data}, loc(loc), dtype(DataType::BOOL), opcode(opcode)
{}
Instruction::Instruction(Opcode opcode, const ModuleLoc *loc)
	: data{.i = 0}, loc(loc), dtype(DataType::NIL), opcode(opcode)
{}

Bytecode::Bytecode() {}
Bytecode::~Bytecode() {}

void Bytecode::dump(OStream &os) const
{
	for(size_t idx = 0; idx < code.size(); ++idx) {
		auto &i = code[idx];
		os << std::left << std::setw(5) << idx << std::left << std::setw(14)
		   << getOpcodeStr(i.getOpcode());
		if(i.isDataNil()) os << "[nil]\n";
		if(i.isDataChr()) os << "[chr]  " << i.getDataChr() << "\n";
		if(i.isDataInt()) os << "[int]  " << i.getDataInt() << "\n";
		if(i.isDataFlt()) os << "[flt]  " << i.getDataFlt() << "\n";
		if(i.isDataStr()) os << "[str]  " << i.getDataStr() << "\n";
		if(i.isDataIden()) os << "[iden] " << i.getDataStr() << "\n";
		if(i.isDataBool()) os << "[bool] " << (i.getDataBool() ? "true" : "false") << "\n";
	}
}

} // namespace fer