#include "Atomic.hpp"

#include "VM/VM.hpp"

namespace fer
{

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarAtomicBool /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarAtomicBool::VarAtomicBool(ModuleLoc loc, bool _val) : Var(loc, 0), val(_val) {}
bool VarAtomicBool::onSet(VirtualMachine &vm, Var *from)
{
    val = as<VarAtomicBool>(from)->getVal();
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarAtomicInt //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarAtomicInt::VarAtomicInt(ModuleLoc loc, int64_t _val) : Var(loc, 0), val(_val) {}
VarAtomicInt::VarAtomicInt(ModuleLoc loc, const char *_val) : Var(loc, 0), val(std::stoll(_val)) {}
bool VarAtomicInt::onSet(VirtualMachine &vm, Var *from)
{
    val = as<VarAtomicInt>(from)->getVal();
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// AtomicBool /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(atomicBoolNew, 1, false,
           "  fn(value) -> AtomicBool\n"
           "Creates a new atomic boolean with `value` and returns it.")
{
    EXPECT(VarBool, args[1], "creating an atomic bool");
    return vm.makeVar<VarAtomicBool>(loc, as<VarBool>(args[1])->getVal());
}

FERAL_FUNC(atomicBoolCopy, 0, false,
           "  var.fn() -> AtomicBool\n"
           "Copies the atomic boolean data and returns it.")
{
    return vm.makeVar<VarAtomicBool>(loc, as<VarAtomicBool>(args[0])->getVal());
}

FERAL_FUNC(atomicBoolSet, 1, false,
           "  var.fn(value) -> Nil\n"
           "Sets the value of atomic bool `var` to be `value`.")
{
    EXPECT(VarBool, args[1], "setting an atomic bool");
    as<VarAtomicBool>(args[0])->setVal(as<VarBool>(args[1])->getVal());
    return vm.getNil();
}

FERAL_FUNC(atomicBoolGet, 0, false,
           "  var.fn() -> Bool\n"
           "Returns the value of atomic bool `var`.")
{
    return as<VarAtomicBool>(args[0])->getVal() ? vm.getTrue() : vm.getFalse();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// AtomicInt /////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(atomicIntNew, 1, false,
           "  fn(value) -> AtomicInt\n"
           "Creates a new atomic integer with `value` and returns it.")
{
    EXPECT(VarInt, args[1], "creating an atomic int");
    return vm.makeVar<VarAtomicInt>(loc, as<VarInt>(args[1])->getVal());
}

FERAL_FUNC(atomicIntCopy, 0, false,
           "  var.fn() -> AtomicInt\n"
           "Copies the atomic integer data and returns it.")
{
    return vm.makeVar<VarAtomicInt>(loc, as<VarAtomicInt>(args[0])->getVal());
}

FERAL_FUNC(atomicIntSet, 1, false,
           "  var.fn(value) -> Nil\n"
           "Sets the value of atomic bool `var` to be `value`.")
{
    EXPECT(VarInt, args[1], "setting an atomic int");
    as<VarAtomicInt>(args[0])->setVal(as<VarInt>(args[1])->getVal());
    return vm.getNil();
}

FERAL_FUNC(atomicIntGet, 0, false,
           "  var.fn() -> Int\n"
           "Returns the value of atomic int `var`.")
{
    return vm.makeVar<VarInt>(loc, as<VarAtomicInt>(args[0])->getVal());
}

INIT_DLL(Atomic)
{
    VarModule *mod = vm.getCurrModule();

    vm.registerType<VarAtomicBool>(loc, "AtomicBool", "An atomic boolean.");
    vm.registerType<VarAtomicInt>(loc, "AtomicInt", "An atomic integer.");

    mod->addNativeFn(vm, "newBool", atomicBoolNew);
    mod->addNativeFn(vm, "newInt", atomicIntNew);

    vm.addNativeTypeFn<VarAtomicBool>(loc, "_copy_", atomicBoolCopy);
    vm.addNativeTypeFn<VarAtomicBool>(loc, "set", atomicBoolSet);
    vm.addNativeTypeFn<VarAtomicBool>(loc, "get", atomicBoolGet);
    vm.addNativeTypeFn<VarAtomicInt>(loc, "_copy_", atomicIntCopy);
    vm.addNativeTypeFn<VarAtomicInt>(loc, "set", atomicIntSet);
    vm.addNativeTypeFn<VarAtomicInt>(loc, "get", atomicIntGet);
    return true;
}

} // namespace fer