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
Instruction::Instruction(Opcode opcode, ModuleLoc loc, int64_t data)
    : data(data), loc(loc), dtype(DataType::INT), opcode(opcode)
{}
Instruction::Instruction(Opcode opcode, ModuleLoc loc, double data)
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
    os << "[" << loc.id << ", " << loc.offStart << " .. " << loc.offEnd << "] ";
    if(isDataNil()) os << "[nil]";
    if(isDataInt()) os << "[int]  " << getDataInt();
    if(isDataFlt()) os << "[flt]  " << getDataFlt();
    if(isDataStr()) os << "[str]  " << getDataStr();
    if(isDataIden()) os << "[iden] " << getDataStr();
    if(isDataBool()) os << "[bool] " << (getDataBool() ? "true" : "false");
}

bool Instruction::readFromFile(FILE *f, Instruction &ins)
{
    fread(&ins.loc, sizeof(ins.loc), 1, f);
    fread(&ins.opcode, sizeof(ins.opcode), 1, f);
    fread(&ins.dtype, sizeof(ins.dtype), 1, f);
    if(ins.isDataInt()) {
        int64_t d;
        fread(&d, sizeof(d), 1, f);
        ins.data = d;
    }
    if(ins.isDataFlt()) {
        double d;
        fread(&d, sizeof(d), 1, f);
        ins.data = d;
    }
    if(ins.isDataStr() || ins.isDataIden()) {
        size_t sz;
        fread(&sz, sizeof(sz), 1, f);
        String d(sz, '0');
        if(sz) fread(d.data(), sizeof(String::value_type), sz, f);
        ins.data = std::move(d);
    }
    if(ins.isDataBool()) {
        bool d;
        fread(&d, sizeof(d), 1, f);
        ins.data = d;
    }
    return true;
}

void Instruction::writeToFile(FILE *f) const
{
    fwrite(&loc, sizeof(loc), 1, f);
    fwrite(&opcode, sizeof(opcode), 1, f);
    fwrite(&dtype, sizeof(dtype), 1, f);
    if(isDataInt()) {
        int64_t d = getDataInt();
        fwrite(&d, sizeof(d), 1, f);
    } else if(isDataFlt()) {
        double d = getDataFlt();
        fwrite(&d, sizeof(d), 1, f);
    } else if(isDataStr() || isDataIden()) {
        StringRef d = getDataStr();
        size_t sz   = d.size();
        fwrite(&sz, sizeof(sz), 1, f);
        if(sz) fwrite(d.data(), sizeof(StringRef::value_type), sz, f);
    } else if(isDataBool()) {
        bool d = getDataBool();
        fwrite(&d, sizeof(d), 1, f);
    }
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

bool Bytecode::readFromFile(FILE *f, Bytecode &bc)
{
    size_t count;
    fread(&count, sizeof(count), 1, f);
    bc.code.reserve(count);
    for(size_t i = 0; i < count; ++i) {
        bc.addInstrNil(Opcode::LAST, {});
        if(!Instruction::readFromFile(f, bc.getInstrAt(i))) return false;
    }
    return true;
}

void Bytecode::writeToFile(FILE *f) const
{
    size_t count = code.size();
    fwrite(&count, sizeof(count), 1, f);
    for(auto &c : code) { c.writeToFile(f); }
}

} // namespace fer