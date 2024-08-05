#include "Ptr.hpp"

#include "VM/Interpreter.hpp"

namespace fer
{

VarPtr::VarPtr(const ModuleLoc *loc, Var *val) : Var(loc, false, false), val(val) { incref(val); }
VarPtr::~VarPtr() { decref(val); }

Var *VarPtr::copyImpl(const ModuleLoc *loc) { return new VarPtr(loc, val); }
void VarPtr::set(Var *from)
{
	decref(val);
	val = as<VarPtr>(from)->val;
	incref(val);
}

void VarPtr::update(Var *with)
{
	decref(val);
	val = with;
	incref(val);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *ptrNewNative(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarPtr>(loc, args[1]);
}

Var *ptrSet(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const StringMap<AssnArgData> &assn_args)
{
	VarPtr *self = as<VarPtr>(args[0]);
	self->update(args[1]);
	return args[0];
}

Var *ptrGet(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const StringMap<AssnArgData> &assn_args)
{
	return as<VarPtr>(args[0])->get();
}

INIT_MODULE(Ptr)
{
	VarModule *mod = vm.getCurrModule();

	vm.registerType<VarPtr>(loc, "Ptr");

	mod->addNativeFn("new", ptrNewNative, 1);

	vm.addNativeTypeFn<VarPtr>(loc, "set", ptrSet, 1);
	vm.addNativeTypeFn<VarPtr>(loc, "get", ptrGet, 0);
	return true;
}

} // namespace fer