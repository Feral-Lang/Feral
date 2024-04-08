#include "std/StructType.hpp"
#include "VM/Interpreter.hpp"

static size_t genStructEnumID()
{
	static size_t id = -1;
	return id--;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *createStruct(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		  const Map<String, AssnArgData> &assn_args)
{
	VarStructDef *res = vm.makeVar<VarStructDef>(loc, assn_args.size(), genStructEnumID());
	for(auto &attr : assn_args) {
		res->setAttr(attr.first, attr.second.val, true);
		res->setAttrOrderAt(attr.second.pos, attr.first);
	}
	return res;
}

Var *createEnum(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		const Map<String, AssnArgData> &assn_args)
{
	VarStruct *res =
	vm.makeVar<VarStruct>(loc, nullptr, args.size() + assn_args.size() - 1, genStructEnumID());

	for(size_t i = 1; i < args.size(); ++i) {
		auto &a = args[i];
		if(!a->is<VarStr>()) {
			vm.fail(a->getLoc(),
				"expected strings for enums (use strings or atoms), found: ",
				vm.getTypeName(a));
			vm.unmakeVar(res);
			return nullptr;
		}
		res->setAttr(as<VarStr>(a)->get(), vm.makeVarWithRef<VarInt>(loc, i - 1), false);
	}

	for(auto &a : assn_args) {
		if(!a.second.val->is<VarInt>()) {
			vm.fail(a.second.val->getLoc(),
				"expected argument value to be of type integer for enums, found: ",
				vm.getTypeName(a.second.val));
			vm.unmakeVar(res);
			return nullptr;
		}
		res->setAttr(a.first, a.second.val->copy(loc), false);
	}

	return res;
}

Var *structToStr(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		 const Map<String, AssnArgData> &assn_args)
{
	VarStruct *st = as<VarStruct>(args[0]);
	VarStr *res   = vm.makeVar<VarStr>(loc, vm.getTypeName(st->getTypeFnID()));
	res->get() += "{";
	for(auto &a : st->getAttrs()) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{a.second};
		if(!vm.callFn(loc, "str", v, tmp, {})) return nullptr;
		if(!v->is<VarStr>()) {
			vm.fail(loc,
				"'str' member call did not return a"
				" string, instead returned: ",
				vm.getTypeName(v));
			decref(v);
			vm.unmakeVar(res);
			return nullptr;
		}
		res->get() += a.first;
		res->get() += ": ";
		res->get() += as<VarStr>(v)->get();
		decref(v);
		res->get() += ", ";
	}
	if(!st->getAttrs().empty()) {
		res->get().pop_back();
		res->get().pop_back();
	}
	res->get() += "}";
	return res;
}

Var *structDefSetTypeName(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
			  const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected typename to be of type string, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	VarStructDef *def  = as<VarStructDef>(args[0]);
	const String &name = as<VarStr>(args[1])->get();
	vm.setTypeName(def->getID(), name);
	return vm.getNil();
}

Var *structDefGetFields(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
			const Map<String, AssnArgData> &assn_args)
{
	VarStructDef *def = as<VarStructDef>(args[0]);
	VarVec *res	  = vm.makeVar<VarVec>(loc, def->getAttrOrder().size(), false);
	for(auto &a : def->getAttrOrder()) {
		res->push(vm.makeVarWithRef<VarStr>(loc, a));
	}
	return res;
}

Var *structDefGetFieldValue(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
			    const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected field name to be "
			"of type string, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	const String &name = as<VarStr>(args[1])->get();
	Var *res	   = as<VarStructDef>(args[0])->getAttr(name);
	return res ? res : vm.getNil();
}

Var *structGetFields(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		     const Map<String, AssnArgData> &assn_args)
{
	VarStruct *st = as<VarStruct>(args[0]);
	VarVec *res   = vm.makeVar<VarVec>(loc, st->getAttrs().size(), false);
	for(auto &a : st->getAttrs()) {
		res->push(vm.makeVarWithRef<VarStr>(loc, a.first));
	}
	return res;
}

Var *structSetFieldValue(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
			 const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(
		loc, "expected field name to be of type string, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	const String &name = as<VarStr>(args[1])->get();

	VarStruct *st = as<VarStruct>(args[0]);
	Var *val      = st->getAttr(name);
	if(!val) {
		vm.fail(loc, "field name: '", name, "' not found in struct");
		return nullptr;
	}

	if(val->getType() != args[2]->getType()) {
		vm.fail(loc, "attribute value type mismatch; expected: '", vm.getTypeName(val),
			"', provided: '", vm.getTypeName(args[2]), "'");
		return nullptr;
	}
	val->set(args[2]);
	return vm.getNil();
}

INIT_MODULE(Lang)
{
	VarModule *mod = vm.getCurrModule();

	mod->addNativeFn("enum", createEnum, 0, true);
	mod->addNativeFn("struct", createStruct);

	vm.addNativeTypeFn<VarStructDef>(loc, "setTypeName", structDefSetTypeName, 1);
	vm.addNativeTypeFn<VarStructDef>(loc, "getFields", structDefGetFields, 0);
	vm.addNativeTypeFn<VarStructDef>(loc, "[]", structDefGetFieldValue, 1);

	vm.addNativeTypeFn<VarStruct>(loc, "getFields", structGetFields, 0);
	vm.addNativeTypeFn<VarStruct>(loc, "setField", structSetFieldValue, 2);
	vm.addNativeTypeFn<VarStruct>(loc, "str", structToStr, 0);

	return true;
}
