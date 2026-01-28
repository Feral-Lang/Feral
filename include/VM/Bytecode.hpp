#pragma once

#include "Error.hpp"

namespace fer
{

class ModuleLoc;

enum class Opcode : uint8_t
{
    LOAD_DATA,     // laod a const int/float/char/string from operand onto the stack
    UNLOAD,        // unload from stack; operand = count of unloads to perform
    STORE,         // store data in a variable; no operand
    CREATE,        // create a variable with name as operand, value present in stack
    CREATE_IN,     // create a variable with name as operand, value and 'in' present in stack
    PUSH_BLOCK,    // push a layer for variables on stack; operand = count of layers to push
    POP_BLOCK,     // pop a layer of variables from stack; operand = count of layers to pop
    PUSH_LOOP,     // special handling for loops
    POP_LOOP,      // special handling for loops
    RETURN,        // self explanatory; operand = true if a val exists, false for void/nil
    BLOCK_TILL,    // create a block (for function); operand = index till which block exists
    CREATE_FN,     // self explanatory; operand = string:
                   // first char '1' if function contains keyword arg, else '0';
                   // second char '1' if function contains variadic, else '0';
                   // rest chars '1' if the equivalent arg has default value, else '0'
    CONTINUE,      // self explanatory; operand = jump index
    BREAK,         // self explanatory, operand = pop loop index
    JMP,           // jump unconditionally; operand = index in bytecode to jump to
    JMP_NIL,       // jump if nil (for for-in loops); operand = index in bytecode to jump to
    JMP_TRUE,      // jump if true; operand = index in bytecode to jump to
    JMP_FALSE,     // jump if false; operand = index in bytecode to jump to
    JMP_TRUE_POP,  // jump if true; operand = index in bytecode to jump to
    JMP_FALSE_POP, // jump if false; operand = index in bytecode to jump to

    // for 'or' block
    PUSH_TRY, // pushes a block alongside an error handler function from exec stack;
              // operand = index of the corresponding POP_TRY
    POP_TRY,  // pops the block alongside the error handler function

    ATTR, // operand = string - attribute name

    // arginfo:
    // 0 => simple
    // 1 => keyword
    // 2 => unpack
    CALL,     // operand = string of arginfo
    MEM_CALL, // operand = string of arginfo

    LAST, // used only as a case in execution
};

StringRef getOpcodeStr(Opcode opcode);

enum class DataType : uint8_t
{
    BOOL,
    NIL,
    INT,
    FLT,
    STR,
    IDEN,
};

class Instruction
{
public:
    using Data = Variant<String, int64_t, double, bool>;

private:
    Data data;
    String comment;
    ModuleLoc loc;
    DataType dtype;
    Opcode opcode;

public:
    Instruction(Opcode opcode, ModuleLoc loc, String &&data, DataType dtype, String &&comment);
    Instruction(Opcode opcode, ModuleLoc loc, int64_t data);
    Instruction(Opcode opcode, ModuleLoc loc, double data);
    Instruction(Opcode opcode, ModuleLoc loc, bool data);
    Instruction(Opcode opcode, ModuleLoc loc); // for nil

#define isDataX(X, ENUMVAL) \
    inline bool isData##X() const { return dtype == DataType::ENUMVAL; }
    isDataX(Bool, BOOL);
    isDataX(Nil, NIL);
    isDataX(Int, INT);
    isDataX(Flt, FLT);
    isDataX(Str, STR);
    isDataX(Iden, IDEN);

    inline void setInt(int64_t dat) { data = dat; }
    inline void setStr(String &&dat) { data = std::move(dat); }
    inline void setComment(String &&dat) { comment = std::move(dat); }

    inline ModuleLoc getLoc() const { return loc; }
    inline StringRef getDataStr() const { return std::get<String>(data); }
    inline int64_t getDataInt() const { return std::get<int64_t>(data); }
    inline double getDataFlt() const { return std::get<double>(data); }
    inline bool getDataBool() const { return std::get<bool>(data); }

    inline const Data &getData() const { return data; }
    inline StringRef getComment() const { return comment; }
    inline bool hasComment() const { return !comment.empty(); }
    inline DataType getDataType() const { return dtype; }
    inline Opcode getOpcode() const { return opcode; }

    void dump(OStream &os) const;

    static bool readFromFile(FILE *f, Instruction &ins);
    void writeToFile(FILE *f) const;
};

class Bytecode
{
    Vector<Instruction> code;

public:
    inline void addInstrStr(Opcode opcode, ModuleLoc loc, String &&data, String &&comment = "")
    {
        code.emplace_back(opcode, loc, std::move(data), DataType::STR, std::move(comment));
    }
    inline void addInstrIden(Opcode opcode, ModuleLoc loc, String &&data, String &&comment = "")
    {
        code.emplace_back(opcode, loc, std::move(data), DataType::IDEN, std::move(comment));
    }
    inline void addInstrInt(Opcode opcode, ModuleLoc loc, int64_t data)
    {
        code.emplace_back(opcode, loc, data);
    }
    inline void addInstrFlt(Opcode opcode, ModuleLoc loc, double data)
    {
        code.emplace_back(opcode, loc, data);
    }
    inline void addInstrBool(Opcode opcode, ModuleLoc loc, bool data)
    {
        code.emplace_back(opcode, loc, data);
    }
    inline void addInstrNil(Opcode opcode, ModuleLoc loc) { code.emplace_back(opcode, loc); }

    inline void updateInstrInt(size_t instrIdx, int64_t data) { code[instrIdx].setInt(data); }
    inline void updateInstrStr(size_t instrIdx, String &&data)
    {
        code[instrIdx].setStr(std::move(data));
    }

    inline void pop() { code.pop_back(); }
    inline void erase(size_t idx) { code.erase(code.begin() + idx); }
    inline size_t getLastIndex() const { return code.size() - 1; }
    inline size_t size() const { return code.size(); }
    inline Instruction &getInstrAt(size_t idx) { return code[idx]; }
    inline const Vector<Instruction> &getBytecode() const { return code; }
    inline const Instruction &getInstrAt(size_t idx) const { return code[idx]; }

    void dump(OStream &os) const;

    static bool readFromFile(FILE *f, Bytecode &bc);
    void writeToFile(FILE *f) const;
};

} // namespace fer
