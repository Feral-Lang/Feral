#include "Ptr.hpp"

#include "VM/Interpreter.hpp"

namespace fer
{

VarPtr::VarPtr(ModuleLoc loc, Var *val) : Var(loc, false, false), val(val) {}
void VarPtr::onCreate(Interpreter &vm) { vm.incVarRef(val); }
void VarPtr::onDestroy(Interpreter &vm) { vm.decVarRef(val); }
Var *VarPtr::onCopy(Interpreter &vm, ModuleLoc loc) { return vm.makeVarWithRef<VarPtr>(loc, val); }
void VarPtr::onSet(Interpreter &vm, Var *from) { setVal(vm, as<VarPtr>(from)->val); }
void VarPtr::setVal(Interpreter &vm, Var *newval)
{
	vm.decVarRef(val);
	val = newval;
	vm.incVarRef(val);
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
	self->setVal(vm, args[1]);
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