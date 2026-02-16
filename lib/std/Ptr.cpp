#include "Ptr.hpp"

#include "VM/VM.hpp"

namespace fer
{

VarPtr::VarPtr(ModuleLoc loc, Var *val) : Var(loc, 0), val(val) {}
void VarPtr::onCreate(VirtualMachine &vm) { vm.incVarRef(val); }
void VarPtr::onDestroy(VirtualMachine &vm) { vm.decVarRef(val); }
bool VarPtr::onSet(VirtualMachine &vm, Var *from) { return setVal(vm, as<VarPtr>(from)->val); }
bool VarPtr::setVal(VirtualMachine &vm, Var *newval)
{
    vm.incVarRef(newval);
    vm.decVarRef(val);
    val = newval;
    return true;
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

FERAL_FUNC(ptrCopy, 0, false,
           "  var.fn() -> Ptr\n"
           "Copies the pointer data and returns it.")
{
    return vm.makeVar<VarPtr>(loc, as<VarPtr>(args[0])->getVal());
}

FERAL_FUNC(ptrSet, 1, false,
           "  var.fn(data) -> var\n"
           "Sets the Ptr `var` to contain `data` and returns `var` itself.")
{
    VarPtr *self = as<VarPtr>(args[0]);
    if(!self->setVal(vm, args[1])) return nullptr;
    return args[0];
}

FERAL_FUNC(ptrGet, 0, false,
           "  var.fn() -> Var\n"
           "Returns the contained data in Ptr `var`.")
{
    return as<VarPtr>(args[0])->getVal();
}

INIT_DLL(Ptr)
{
    VarModule *mod = vm.getCurrModule();

    vm.registerType<VarPtr>(loc, "Ptr",
                            "A reference container type which can contain and get other data.");

    mod->addNativeFn(vm, "new", ptrNewNative);

    vm.addNativeTypeFn<VarPtr>(loc, "_copy_", ptrCopy);
    vm.addNativeTypeFn<VarPtr>(loc, "set", ptrSet);
    vm.addNativeTypeFn<VarPtr>(loc, "get", ptrGet);
    return true;
}

} // namespace fer