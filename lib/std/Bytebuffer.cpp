#include "BytebufferType.hpp"
#include "VM/Interpreter.hpp"

namespace fer
{

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *bytebufferNewNative(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
			 const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(
		loc, "expected int argument for bytebuffer size, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	return vm.makeVar<VarBytebuffer>(loc, as<VarInt>(args[1])->get());
}

Var *bytebufferResize(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		      const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc,
			"expected int argument "
			"for bytebuffer size, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	VarBytebuffer *self = as<VarBytebuffer>(args[0]);
	self->resize(as<VarInt>(args[1])->get());
	return args[0];
}

Var *bytebufferSetLen(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		      const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(
		loc, "expected int argument for bytebuffer len, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	VarBytebuffer *self = as<VarBytebuffer>(args[0]);
	self->setLen(as<VarInt>(args[1])->get());
	return args[0];
}

Var *bytebufferCapacity(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
			const Map<String, AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, as<VarBytebuffer>(args[0])->capacity());
}

Var *bytebufferLen(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		   const Map<String, AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, as<VarBytebuffer>(args[0])->len());
}

Var *bytebufferToStr(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		     const Map<String, AssnArgData> &assn_args)
{
	VarBytebuffer *self = as<VarBytebuffer>(args[0]);
	if(self->len() == 0) return vm.makeVar<VarStr>(loc, "");
	return vm.makeVar<VarStr>(loc, self->getBuf(), self->len());
}

INIT_MODULE(Bytebuffer)
{
	VarModule *mod = vm.getCurrModule();

	mod->addNativeFn("newNative", bytebufferNewNative, 1);

	vm.registerType<VarBytebuffer>(loc, "Bytebuffer");

	vm.addNativeTypeFn<VarBytebuffer>(loc, "resize", bytebufferResize, 1);
	vm.addNativeTypeFn<VarBytebuffer>(loc, "setLen", bytebufferSetLen, 1);
	vm.addNativeTypeFn<VarBytebuffer>(loc, "len", bytebufferLen, 0);
	vm.addNativeTypeFn<VarBytebuffer>(loc, "capacity", bytebufferCapacity, 0);
	vm.addNativeTypeFn<VarBytebuffer>(loc, "str", bytebufferToStr, 0);
	return true;
}

} // namespace fer