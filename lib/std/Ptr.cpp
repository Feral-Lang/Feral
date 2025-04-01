#include "Ptr.hpp"

#include "VM/InterpreterThread.hpp"

namespace fer
{

VarPtr::VarPtr(ModuleLoc loc, Var *val) : Var(loc, false, false), val(val) {}
void VarPtr::onCreate(InterpreterState &vms) { vms.incVarRef(val); }
void VarPtr::onDestroy(InterpreterState &vms) { vms.decVarRef(val); }
Var *VarPtr::onCopy(InterpreterState &vms, ModuleLoc loc)
{
	return vms.makeVarWithRef<VarPtr>(loc, val);
}
void VarPtr::onSet(InterpreterState &vms, Var *from) { setVal(vms, as<VarPtr>(from)->val); }
void VarPtr::setVal(InterpreterState &vms, Var *newval)
{
	vms.decVarRef(val);
	val = newval;
	vms.incVarRef(val);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *ptrNewNative(InterpreterThread &vm, ModuleLoc loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarPtr>(loc, args[1]);
}

Var *ptrSet(InterpreterThread &vm, ModuleLoc loc, Span<Var *> args,
	    const StringMap<AssnArgData> &assn_args)
{
	VarPtr *self = as<VarPtr>(args[0]);
	self->setVal(vm.getGlobalState(), args[1]);
	return args[0];
}

Var *ptrGet(InterpreterThread &vm, ModuleLoc loc, Span<Var *> args,
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