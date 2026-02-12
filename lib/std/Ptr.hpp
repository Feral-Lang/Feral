#pragma once

#include "VM/VarTypes.hpp"

namespace fer
{

class VarPtr : public Var
{
    Var *val;

    void onCreate(MemoryManager &mem) override;
    void onDestroy(MemoryManager &mem) override;
    bool onSet(VirtualMachine &vm, Var *from) override;

public:
    VarPtr(ModuleLoc loc, Var *val);

    bool setVal(VirtualMachine &vm, Var *newval);

    inline Var *getVal() { return val; }
};

} // namespace fer