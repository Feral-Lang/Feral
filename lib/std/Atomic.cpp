#include "Atomic.hpp"

#include "VM/Interpreter.hpp"

namespace fer
{

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarAtomicBool /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarAtomicBool::VarAtomicBool(ModuleLoc loc, bool _val) : Var(loc, false, false), val(_val) {}
Var *VarAtomicBool::onCopy(MemoryManager &mem, ModuleLoc loc)
{
    return makeVarWithRef<VarAtomicBool>(mem, loc, val);
}
void VarAtomicBool::onSet(MemoryManager &mem, Var *from)
{
    val = as<VarAtomicBool>(from)->getVal();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarAtomicInt //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarAtomicInt::VarAtomicInt(ModuleLoc loc, int64_t _val) : Var(loc, false, false), val(_val) {}
VarAtomicInt::VarAtomicInt(ModuleLoc loc, const char *_val)
    : Var(loc, false, false), val(std::stoll(_val))
{}
Var *VarAtomicInt::onCopy(MemoryManager &mem, ModuleLoc loc)
{
    return makeVarWithRef<VarAtomicInt>(mem, loc, val);
}
void VarAtomicInt::onSet(MemoryManager &mem, Var *from) { val = as<VarAtomicInt>(from)->getVal(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// AtomicBool /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *atomicBoolNew(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                   const StringMap<AssnArgData> &assnArgs)
{
    if(!args[1]->is<VarBool>()) {
        vm.fail(loc, "expected bool argument for creating an atomic bool, found: ",
                vm.getTypeName(args[1]));
        return nullptr;
    }
    return vm.makeVar<VarAtomicBool>(loc, as<VarBool>(args[1])->getVal());
}

Var *atomicBoolSet(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                   const StringMap<AssnArgData> &assnArgs)
{
    if(!args[1]->is<VarBool>()) {
        vm.fail(loc, "expected int argument for setting an atomic int, found: ",
                vm.getTypeName(args[1]));
        return nullptr;
    }
    as<VarAtomicBool>(args[0])->setVal(as<VarBool>(args[1])->getVal());
    return vm.getNil();
}

Var *atomicBoolGet(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                   const StringMap<AssnArgData> &assnArgs)
{
    return as<VarAtomicBool>(args[0])->getVal() ? vm.getTrue() : vm.getFalse();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// AtomicInt /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *atomicIntNew(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                  const StringMap<AssnArgData> &assnArgs)
{
    if(!args[1]->is<VarInt>()) {
        vm.fail(loc, "expected int argument for creating an atomic int, found: ",
                vm.getTypeName(args[1]));
        return nullptr;
    }
    return vm.makeVar<VarAtomicInt>(loc, as<VarInt>(args[1])->getVal());
}

Var *atomicIntSet(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                  const StringMap<AssnArgData> &assnArgs)
{
    if(!args[1]->is<VarInt>()) {
        vm.fail(loc, "expected int argument for setting an atomic int, found: ",
                vm.getTypeName(args[1]));
        return nullptr;
    }
    as<VarAtomicInt>(args[0])->setVal(as<VarInt>(args[1])->getVal());
    return vm.getNil();
}

Var *atomicIntGet(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                  const StringMap<AssnArgData> &assnArgs)
{
    return vm.makeVar<VarInt>(loc, as<VarAtomicInt>(args[0])->getVal());
}

INIT_MODULE(Atomic)
{
    VarModule *mod = vm.getCurrModule();

    vm.registerType<VarAtomicBool>(loc, "AtomicBool");
    vm.registerType<VarAtomicInt>(loc, "AtomicInt");

    mod->addNativeFn(vm, "newBool", atomicBoolNew, 1);
    mod->addNativeFn(vm, "newInt", atomicIntNew, 1);

    vm.addNativeTypeFn<VarAtomicBool>(loc, "set", atomicBoolSet, 1);
    vm.addNativeTypeFn<VarAtomicBool>(loc, "get", atomicBoolGet, 0);
    vm.addNativeTypeFn<VarAtomicInt>(loc, "set", atomicIntSet, 1);
    vm.addNativeTypeFn<VarAtomicInt>(loc, "get", atomicIntGet, 0);
    return true;
}

} // namespace fer