#include "PtrType.hpp"

namespace fer
{

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *ptrNewNative(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		  const Map<String, AssnArgData> &assn_args)
{
	return vm.makeVar<VarPtr>(loc, args[1]);
}

Var *ptrSet(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const Map<String, AssnArgData> &assn_args)
{
	VarPtr *self = as<VarPtr>(args[0]);
	self->update(args[1]);
	return args[0];
}

Var *ptrGet(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const Map<String, AssnArgData> &assn_args)
{
	return as<VarPtr>(args[0])->get();
}

INIT_MODULE(Ptr)
{
	VarModule *mod = vm.getCurrModule();
	mod->addNativeFn("new", ptrNewNative, 1);

	vm.addNativeTypeFn<VarPtr>(loc, "set", ptrSet, 1);
	vm.addNativeTypeFn<VarPtr>(loc, "get", ptrGet, 0);
	return true;
}

} // namespace fer