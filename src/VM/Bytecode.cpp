#include "VM/Bytecode.hpp"

#include <iomanip>

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
    case Opcode::JMP_NIL: return "JMP_NIL";
    case Opcode::JMP_TRUE: return "JMP_TRUE";
    case Opcode::JMP_FALSE: return "JMP_FALSE";
    case Opcode::JMP_TRUE_POP: return "JMP_TRUE_POP";
    case Opcode::JMP_FALSE_POP: return "JMP_FALSE_POP";
    case Opcode::PUSH_TRY: return "PUSH_TRY";
    case Opcode::POP_TRY: return "POP_TRY";
    case Opcode::ATTR: return "ATTR";
    case Opcode::CALL: return "FNCALL";
    case Opcode::MEM_CALL: return "MEM_FNCALL";
    default: break;
    }
    return "";
}

Instruction::Instruction(Opcode opcode, ModuleLoc loc, String &&data, DataType dtype)
    : data(std::move(data)), loc(loc), dtype(dtype), opcode(opcode)
{}
Instruction::Instruction(Opcode opcode, ModuleLoc loc, StringRef data, DataType dtype)
    : data(String(data)), loc(loc), dtype(dtype), opcode(opcode)
{}
Instruction::Instruction(Opcode opcode, ModuleLoc loc, int64_t data)
    : data(data), loc(loc), dtype(DataType::INT), opcode(opcode)
{}
Instruction::Instruction(Opcode opcode, ModuleLoc loc, long double data)
    : data(data), loc(loc), dtype(DataType::FLT), opcode(opcode)
{}
Instruction::Instruction(Opcode opcode, ModuleLoc loc, bool data)
    : data(data), loc(loc), dtype(DataType::BOOL), opcode(opcode)
{}
Instruction::Instruction(Opcode opcode, ModuleLoc loc)
    : loc(loc), dtype(DataType::NIL), opcode(opcode)
{}

void Instruction::dump(OStream &os) const
{
    os << getOpcodeStr(getOpcode());
    if(isDataNil()) os << "[nil]";
    if(isDataInt()) os << "[int]  " << getDataInt();
    if(isDataFlt()) os << "[flt]  " << getDataFlt();
    if(isDataStr()) os << "[str]  " << getDataStr();
    if(isDataIden()) os << "[iden] " << getDataStr();
    if(isDataBool()) os << "[bool] " << (getDataBool() ? "true" : "false");
}

void Bytecode::dump(OStream &os) const
{
    for(size_t idx = 0; idx < code.size(); ++idx) {
        auto &i = code[idx];
        os << std::left << std::setw(5) << idx << std::left << std::setw(14);
        i.dump(os);
        os << "\n";
    }
}

} // namespace fer