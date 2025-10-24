#pragma once

#include "VM/VarTypes.hpp"

namespace fer
{

class VarAtomicBool : public Var
{
    Atomic<bool> val;

    Var *onCopy(MemoryManager &mem, ModuleLoc loc) override;
    void onSet(MemoryManager &mem, Var *from) override;

public:
    VarAtomicBool(ModuleLoc loc, bool _val);

    inline void setVal(bool newval) { val.store(newval); }
    inline bool getVal() { return val.load(); }
};

class VarAtomicInt : public Var
{
    Atomic<int64_t> val;

    Var *onCopy(MemoryManager &mem, ModuleLoc loc) override;
    void onSet(MemoryManager &mem, Var *from) override;

public:
    VarAtomicInt(ModuleLoc loc, int64_t _val);
    VarAtomicInt(ModuleLoc loc, const char *_val);

    inline void setVal(int64_t newval) { val.store(newval); }
    inline int64_t getVal() { return val.load(); }
};

} // namespace fer