#include "Ptr.hpp"

#include "VM/Interpreter.hpp"

namespace fer
{

VarPtr::VarPtr(ModuleLoc loc, Var *val) : Var(loc, 0), val(val) {}
void VarPtr::onCreate(MemoryManager &mem) { Var::incVarRef(val); }
void VarPtr::onDestroy(MemoryManager &mem) { Var::decVarRef(mem, val); }
Var *VarPtr::onCopy(MemoryManager &mem, ModuleLoc loc)
{
    return incVarRef(makeVar<VarPtr>(mem, loc, val));
}
void VarPtr::onSet(MemoryManager &mem, Var *from) { setVal(mem, as<VarPtr>(from)->val); }
void VarPtr::setVal(MemoryManager &mem, Var *newval)
{
    Var::decVarRef(mem, val);
    val = newval;
    Var::incVarRef(val);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(ptrNewNative, 1, false,
           "  fn(data) -> Ptr\n"
           "Creates and returns an instance of a Ptr containing `data`.")
{
    return vm.makeVar<VarPtr>(loc, args[1]);
}

FERAL_FUNC(ptrSet, 1, false,
           "  var.fn(data) -> var\n"
           "Sets the Ptr `var` to contain `data` and returns `var` itself.")
{
    VarPtr *self = as<VarPtr>(args[0]);
    self->setVal(vm.getMemoryManager(), args[1]);
    return args[0];
}

FERAL_FUNC(ptrGet, 0, false,
           "  var.fn() -> Var\n"
           "Returns the contained data in Ptr `var`.")
{
    return as<VarPtr>(args[0])->getVal();
}

INIT_MODULE(Ptr)
{
    VarModule *mod = vm.getCurrModule();

    vm.registerType<VarPtr>(loc, "Ptr",
                            "A reference container type which can contain and get other data.");

    mod->addNativeFn(vm, "new", ptrNewNative);

    vm.addNativeTypeFn<VarPtr>(loc, "set", ptrSet);
    vm.addNativeTypeFn<VarPtr>(loc, "get", ptrGet);
    return true;
}

} // namespace fer