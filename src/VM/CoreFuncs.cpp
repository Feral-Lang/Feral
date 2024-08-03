#include "VM/CoreFuncs.hpp"

namespace fer
{

bool loadCommon(Interpreter &vm, const ModuleLoc *loc, Var *modname, bool isImport, String &result)
{
	if(!modname->is<VarStr>()) {
		vm.fail(loc,
			"expected argument to be of type string, found: ", vm.getTypeName(modname));
		return false;
	}
	Array<Var *, 3> tmpArgs{nullptr, modname, isImport ? vm.getTrue() : vm.getFalse()};
	Var *ret = nullptr;
	for(auto &callable : vm.getModuleFinders()->get()) {
		if(!vm.callVar(loc, "loadlib", callable, ret, tmpArgs, {})) {
			vm.fail(
			loc, "failed to load module '", as<VarStr>(modname)->get(),
			"' when attempting to call the callable: ", vm.getTypeName(callable));
			return false;
		}
		if(ret && ret->is<VarStr>()) break;
		if(ret && !ret->is<VarNil>()) decref(ret);
		ret = nullptr;
	}
	if(!ret || !ret->is<VarStr>()) {
		vm.fail(loc, "failed to find module: ", as<VarStr>(modname)->get());
		return false;
	}
	result = as<VarStr>(ret)->get();
	decref(ret);
	return true;
}

Var *loadFile(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	String file;
	if(!loadCommon(vm, loc, args[1], true, file)) return nullptr;
	if(!vm.hasModule(file)) {
		int res = vm.compileAndRun(loc, file);
		if(res != 0 && !vm.isExitCalled()) {
			vm.fail(args[1]->getLoc(), "could not import: '", file,
				"', look at error above (exit code: ", res, ")");
			return nullptr;
		}
	}
	return vm.getModule(file);
}

Var *loadLibrary(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		 const StringMap<AssnArgData> &assn_args)
{
	String file;
	if(!loadCommon(vm, loc, args[1], false, file)) return nullptr;
	if(!vm.loadNativeModule(loc, file, as<VarStr>(args[1])->get())) {
		vm.fail(loc, "failed to load module: ", as<VarStr>(args[1])->get());
		return nullptr;
	}
	return vm.getNil();
}

void setupCoreFuncs(Interpreter &vm, const ModuleLoc *loc)
{
	vm.addNativeFn(loc, "import", loadFile, 1);
	vm.addNativeFn(loc, "loadlib", loadLibrary, 1);
}

Var *basicModuleFinder(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		       const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected argument to be of type string, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarBool>()) {
		vm.fail(loc,
			"expected argument to be of type bool, found: ", vm.getTypeName(args[2]));
		return nullptr;
	}
	String modfile = as<VarStr>(args[1])->get();
	bool isImport  = as<VarBool>(args[2])->get();
	if(isImport) {
		if(!vm.findImportModuleIn(vm.getModuleDirs(), modfile)) return vm.getNil();
	} else {
		if(!vm.findNativeModuleIn(vm.getModuleDirs(), modfile)) return vm.getNil();
	}
	return vm.makeVar<VarStr>(loc, modfile);
}

} // namespace fer