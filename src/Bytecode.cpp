#include "Bytecode.hpp"

#include <iomanip>

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
	case Opcode::LOAD_ARG: return "LOAD_ARG";
	case Opcode::RETURN: return "RETURN";
	case Opcode::BLOCK_TILL: return "BLOCK_TILL";
	case Opcode::CREATE_FN: return "CREATE_FN";
	case Opcode::CONTINUE: return "CONTINUE";
	case Opcode::BREAK: return "BREAK";
	case Opcode::JMP: return "JMP";
	case Opcode::JMP_TRUE: return "JMP_TRUE";
	case Opcode::JMP_FALSE: return "JMP_FALSE";
	case Opcode::JMP_POP: return "JMP_POP";
	case Opcode::JMP_TRUE_POP: return "JMP_TRUE_POP";
	case Opcode::JMP_FALSE_POP: return "JMP_FALSE_POP";

	case Opcode::ASSN: return "ASSN";
	// Arithmetic
	case Opcode::ADD: return "ADD";
	case Opcode::SUB: return "SUB";
	case Opcode::MUL: return "MUL";
	case Opcode::DIV: return "DIV";
	case Opcode::MOD: return "MOD";
	case Opcode::ADD_ASSN: return "ADD_ASSN";
	case Opcode::SUB_ASSN: return "SUB_ASSN";
	case Opcode::MUL_ASSN: return "MUL_ASSN";
	case Opcode::DIV_ASSN: return "DIV_ASSN";
	case Opcode::MOD_ASSN: return "MOD_ASSN";
	// Post/Pre Inc/Dec
	case Opcode::XINC: return "XINC";
	case Opcode::INCX: return "INCX";
	case Opcode::XDEC: return "XDEC";
	case Opcode::DECX: return "DECX";
	// Unary
	case Opcode::UADD: return "UADD";
	case Opcode::USUB: return "USUB";
	case Opcode::UAND: return "UAND";
	case Opcode::UMUL: return "UMUL";
	// Logic (LAND and LOR are handled using jmps)
	case Opcode::LNOT: return "LNOT";
	// Comparison
	case Opcode::EQ: return "EQ";
	case Opcode::LT: return "LT";
	case Opcode::GT: return "GT";
	case Opcode::LE: return "LE";
	case Opcode::GE: return "GE";
	case Opcode::NE: return "NE";
	// Bitwise
	case Opcode::BAND: return "BAND";
	case Opcode::BOR: return "BOR";
	case Opcode::BNOT: return "BNOT";
	case Opcode::BXOR: return "BXOR";
	case Opcode::BAND_ASSN: return "BAND_ASSN";
	case Opcode::BOR_ASSN: return "BOR_ASSN";
	case Opcode::BNOT_ASSN: return "BNOT_ASSN";
	case Opcode::BXOR_ASSN: return "BXOR_ASSN";
	// Others
	case Opcode::LSHIFT: return "LSHIFT";
	case Opcode::RSHIFT: return "RSHIFT";
	case Opcode::LSHIFT_ASSN: return "LSHIFT_ASSN";
	case Opcode::RSHIFT_ASSN: return "RSHIFT_ASSN";
	case Opcode::SUBS: return "SUBS";
	case Opcode::DOT: return "DOT";
	case Opcode::FNCALL: return "FNCALL";
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
Instruction::Instruction(Opcode opcode, const ModuleLoc *loc, bool data)
	: data{.b = data}, loc(loc), dtype(DataType::BOOL), opcode(opcode)
{}

Bytecode::Bytecode() {}
Bytecode::~Bytecode() {}

void Bytecode::dump(OStream &os) const
{
	for(size_t idx = 0; idx < code.size(); ++idx) {
		auto &i = code[idx];
		os << std::left << std::setw(5) << idx << std::left << std::setw(14)
		   << getOpcodeStr(i.getOpcode());
		if(i.isInt()) os << "[int]  " << i.getDataInt() << "\n";
		if(i.isFlt()) os << "[flt]  " << i.getDataFlt() << "\n";
		if(i.isChr()) os << "[chr]  " << i.getDataChr() << "\n";
		if(i.isStr()) os << "[str]  " << i.getDataStr() << "\n";
		if(i.isBool()) os << "[bool] " << (i.getDataBool() ? "true" : "false") << "\n";
	}
}

} // namespace fer