#include "std/MapType.hpp"

namespace fer
{

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *mapNew(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const StringMap<AssnArgData> &assn_args)
{
	if((args.size() - 1) % 2 != 0) {
		vm.fail(loc, "argument count must be even to create a map");
		return nullptr;
	}
	auto refloc    = assn_args.find("refs");
	auto caploc    = assn_args.find("cap");
	size_t resvcap = args.size() - 1;
	bool refs      = false;
	if(refloc != assn_args.end()) {
		Var *refsv = refloc->second.val;
		if(!refsv->is<VarBool>()) {
			vm.fail(loc,
				"expected 'refs' named argument to be"
				" of type bool for vec.new(), found: ",
				vm.getTypeName(refsv));
			return nullptr;
		}
		refs = as<VarBool>(refsv)->get();
	}
	if(caploc != assn_args.end()) {
		Var *capv = caploc->second.val;
		if(!capv->is<VarBool>()) {
			vm.fail(loc,
				"expected 'cap' named argument to be"
				" of type int for vec.new(), found: ",
				vm.getTypeName(capv));
			return nullptr;
		}
		resvcap = as<VarInt>(capv)->get();
	}
	StringMap<Var *> mapval;
	mapval.reserve(resvcap);
	for(size_t i = 1; i < args.size(); ++i) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{args[i]};
		if(!vm.callFn(loc, "str", v, tmp, {})) return nullptr;
		if(!v->is<VarStr>()) {
			vm.fail(loc,
				"'str' member call did not return a"
				" string, instead returned: ",
				vm.getTypeName(v));
			decref(v);
			return nullptr;
		}
		const String &key = as<VarStr>(v)->get();
		auto maploc	  = mapval.find(key);
		if(maploc != mapval.end()) decref(maploc->second);
		if(refs) {
			incref(args[++i]);
			mapval[key] = args[i];
		} else {
			mapval[key] = args[++i]->copy(loc);
		}
		decref(v);
	}
	return vm.makeVar<VarMap>(loc, std::move(mapval), refs);
}

Var *mapSize(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, as<VarMap>(args[0])->get().size());
}

Var *mapIsRef(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	return as<VarMap>(args[0])->isRefMap() ? vm.getTrue() : vm.getFalse();
}

Var *mapEmpty(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	return as<VarMap>(args[0])->get().empty() ? vm.getTrue() : vm.getFalse();
}

Var *mapInsert(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const StringMap<AssnArgData> &assn_args)
{
	StringMap<Var *> &map = as<VarMap>(args[0])->get();
	Var *v		      = nullptr;
	Array<Var *, 1> tmp{args[1]};
	if(!vm.callFn(loc, "str", v, tmp, {})) return nullptr;
	if(!v->is<VarStr>()) {
		vm.fail(loc,
			"'str' member call did not return a"
			" string, instead returned: ",
			vm.getTypeName(v));
		decref(v);
		return nullptr;
	}
	const String &key = as<VarStr>(v)->get();
	if(map.find(key) != map.end()) {
		decref(map[key]);
	}
	if(as<VarMap>(args[0])->isRefMap()) {
		incref(args[2]);
		map[key] = args[2];
	} else {
		map[key] = args[2]->copy(loc);
	}
	decref(v);
	return args[0];
}

Var *mapErase(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	StringMap<Var *> &map = as<VarMap>(args[0])->get();
	Var *v		      = nullptr;
	Array<Var *, 1> tmp{args[1]};
	if(!vm.callFn(loc, "str", v, tmp, {})) return nullptr;
	if(!v->is<VarStr>()) {
		vm.fail(loc,
			"'str' member call did not return a"
			" string, instead returned: ",
			vm.getTypeName(v));
		decref(v);
		return nullptr;
	}
	auto maploc = map.find(as<VarStr>(v)->get());
	if(maploc != map.end()) {
		decref(maploc->second);
		map.erase(maploc);
	}
	decref(v);
	return args[0];
}

Var *mapAt(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	   const StringMap<AssnArgData> &assn_args)
{
	StringMap<Var *> &map = as<VarMap>(args[0])->get();
	Var *v		      = nullptr;
	Array<Var *, 1> tmp{args[1]};
	if(!vm.callFn(loc, "str", v, tmp, {})) return nullptr;
	if(!v->is<VarStr>()) {
		vm.fail(loc,
			"'str' member call did not return a"
			" string, instead returned: ",
			vm.getTypeName(v));
		decref(v);
		return nullptr;
	}
	auto maploc = map.find(as<VarStr>(v)->get());
	decref(v);
	return maploc == map.end() ? vm.getNil() : maploc->second;
}

Var *mapFind(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const StringMap<AssnArgData> &assn_args)
{
	StringMap<Var *> &map = as<VarMap>(args[0])->get();
	Var *v		      = nullptr;
	Array<Var *, 1> tmp{args[1]};
	if(!vm.callFn(loc, "str", v, tmp, {})) return nullptr;
	if(!v->is<VarStr>()) {
		vm.fail(loc,
			"'str' member call did not return a"
			" string, instead returned: ",
			vm.getTypeName(v));
		decref(v);
		return nullptr;
	}
	auto maploc = map.find(as<VarStr>(v)->get());
	decref(v);
	return maploc == map.end() ? vm.getFalse() : vm.getTrue();
}

Var *mapEach(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarMapIterator>(loc, as<VarMap>(args[0]));
}

Var *mapIteratorNext(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		     const StringMap<AssnArgData> &assn_args)
{
	VarMapIterator *it = as<VarMapIterator>(args[0]);
	Var *res	   = nullptr;
	if(!it->next(res, vm, loc)) return vm.getNil();
	res->setLoadAsRef();
	return res;
}

INIT_MODULE(Map)
{
	VarModule *mod = vm.getCurrModule();

	vm.registerType<VarMapIterator>(loc, "MapIterator");

	mod->addNativeFn("new", mapNew, 0, true);

	vm.addNativeTypeFn<VarMap>(loc, "len", mapSize, 0);
	vm.addNativeTypeFn<VarMap>(loc, "isRef", mapIsRef, 0);
	vm.addNativeTypeFn<VarMap>(loc, "empty", mapEmpty, 0);
	vm.addNativeTypeFn<VarMap>(loc, "insert", mapInsert, 2);
	vm.addNativeTypeFn<VarMap>(loc, "erase", mapErase, 1);
	vm.addNativeTypeFn<VarMap>(loc, "find", mapFind, 1);
	vm.addNativeTypeFn<VarMap>(loc, "at", mapAt, 1);
	vm.addNativeTypeFn<VarMap>(loc, "[]", mapAt, 1);

	vm.addNativeTypeFn<VarMap>(loc, "each", mapEach, 0);
	vm.addNativeTypeFn<VarMapIterator>(loc, "next", mapIteratorNext, 0);

	return true;
}

} // namespace fer