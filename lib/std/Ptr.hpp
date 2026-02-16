#pragma once

#include "VM/VarTypes.hpp"

namespace fer
{

class VarPtr : public Var
{
    Var *val;

    void onCreate(VirtualMachine &vm) override;
    void onDestroy(VirtualMachine &vm) override;
    bool onSet(VirtualMachine &vm, Var *from) override;

public:
    VarPtr(ModuleLoc loc, Var *val);

    bool setVal(VirtualMachine &vm, Var *newval);

    inline Var *getVal() { return val; }
};

} // namespace fer