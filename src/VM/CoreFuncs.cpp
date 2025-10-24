#include "VM/CoreFuncs.hpp"

namespace fer
{

static RecursiveMutex loadMtx;

bool loadCommon(VirtualMachine &vm, ModuleLoc loc, Var *modname, bool isImport, String &result)
{
	if(!modname->is<VarStr>()) {
		vm.fail(loc,
			"expected argument to be of type string, found: ", vm.getTypeName(modname));
		return false;
	}
	Array<Var *, 3> tmpArgs{nullptr, modname, isImport ? vm.getTrue() : vm.getFalse()};
	Var *ret = nullptr;
	for(auto &callable : vm.getModuleFinders()->getVal()) {
		if(!(ret = vm.callVar(loc, "loadCommon", callable, tmpArgs, {}))) {
			vm.fail(
			loc, "failed to load module '", as<VarStr>(modname)->getVal(),
			"' when attempting to call the callable: ", vm.getTypeName(callable));
			return false;
		}
		if(ret && ret->is<VarStr>()) break;
		if(ret && !ret->is<VarNil>()) vm.decVarRef(ret);
		ret = nullptr;
	}
	if(!ret || !ret->is<VarStr>()) {
		vm.fail(loc, "failed to find module: ", as<VarStr>(modname)->getVal());
		return false;
	}
	result = as<VarStr>(ret)->getVal();
	vm.decVarRef(ret);

	if(isImport) {
		size_t nameLoc = result.rfind(as<VarStr>(modname)->getVal());
		// nameLoc cannot be String::npos since result is the string where modname was
		// found.
		// - 1 for the last slash in the path.
		String dir = result.substr(0, nameLoc - 1);
		vm.tryAddModulePathsFromDir(dir);
	}
	return true;
}

Var *loadFile(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	LockGuard<RecursiveMutex> _(loadMtx);
	String file;
	if(!loadCommon(vm, loc, args[1], true, file)) return nullptr;
	if(!vm.hasModule(file)) {
		int res = vm.compileAndRun(loc, file.c_str());
		if(res != 0 && !vm.isExitCalled()) {
			vm.fail(args[1]->getLoc(), "could not import: '", file,
				"', look at error above (exit code: ", res, ")");
			return nullptr;
		}
	}
	return vm.getModule(file);
}

Var *loadLibrary(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
		 const StringMap<AssnArgData> &assn_args)
{
	LockGuard<RecursiveMutex> _(loadMtx);
	String file;
	if(!loadCommon(vm, loc, args[1], false, file)) return nullptr;
	if(!vm.loadNativeModule(loc, file, as<VarStr>(args[1])->getVal())) {
		vm.fail(loc, "failed to load module: ", as<VarStr>(args[1])->getVal());
		return nullptr;
	}
	return vm.getNil();
}

void setupCoreFuncs(Interpreter &ip, ModuleLoc loc)
{
	ip.addNativeFn(loc, "import", loadFile, 1);
	ip.addNativeFn(loc, "loadlib", loadLibrary, 1);
}

Var *basicModuleFinder(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
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
	String modfile = as<VarStr>(args[1])->getVal();
	bool isImport  = as<VarBool>(args[2])->getVal();
	if(isImport) {
		if(!vm.findImportModuleIn(vm.getModuleDirs(), modfile)) return vm.getNil();
	} else {
		if(!vm.findNativeModuleIn(vm.getModuleDirs(), modfile)) return vm.getNil();
	}
	return vm.makeVar<VarStr>(loc, modfile);
}

Var *basicErrorHandler(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
		       const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarFailure>()) {
		vm.fail(loc, "expected argument of type failure, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	VarFailure *f	      = as<VarFailure>(args[1]);
	Span<ModuleLoc> trace = f->getTrace();
	StringRef msg	      = f->getMsg();
	if(msg.empty()) msg = "unknown failure";
	for(size_t i = 0; i < trace.size(); ++i) {
		err.outStr(trace[i], i != trace.size() - 1 ? "" : msg);
	}
	return nullptr;
}

} // namespace fer