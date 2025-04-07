#include "Ptr.hpp"

#include "VM/Interpreter.hpp"

namespace fer
{

VarPtr::VarPtr(ModuleLoc loc, Var *val) : Var(loc, false, false), val(val) {}
void VarPtr::onCreate(MemoryManager &mem) { Var::incVarRef(val); }
void VarPtr::onDestroy(MemoryManager &mem) { Var::decVarRef(mem, val); }
Var *VarPtr::onCopy(MemoryManager &mem, ModuleLoc loc)
{
	return Var::makeVarWithRef<VarPtr>(mem, loc, val);
}
void VarPtr::onSet(MemoryManager &mem, Var *from) { setVal(mem, as<VarPtr>(from)->val); }
void VarPtr::setVal(MemoryManager &mem, Var *newval)
{
	Var::ScopedThreadLock _(this);
	Var::decVarRef(mem, val);
	val = newval;
	Var::incVarRef(val);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *ptrNewNative(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarPtr>(loc, args[1]);
}

Var *ptrSet(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
	    const StringMap<AssnArgData> &assn_args)
{
	VarPtr *self = as<VarPtr>(args[0]);
	self->setVal(vm.getMemoryManager(), args[1]);
	return args[0];
}

Var *ptrGet(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
	    const StringMap<AssnArgData> &assn_args)
{
	return as<VarPtr>(args[0])->getVal();
}

INIT_MODULE(Ptr)
{
	VarModule *mod = vm.getCurrModule();

	vm.registerType<VarPtr>(loc, "Ptr");

	mod->addNativeFn(vm, "new", ptrNewNative, 1);

	vm.addNativeTypeFn<VarPtr>(loc, "set", ptrSet, 1);
	vm.addNativeTypeFn<VarPtr>(loc, "get", ptrGet, 0);
	return true;
}

} // namespace fer