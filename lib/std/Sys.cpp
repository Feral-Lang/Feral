#include "Config.hpp"
#include "VM/Interpreter.hpp"

using namespace fer;

Var *_exit(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	   const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc, "expected integer for exit code, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	vm.setExitCalled(true);
	vm.setExitCode(mpz_get_si(as<VarInt>(args[1])->get()));
	return vm.getNil();
}

Var *varExists(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for variable name, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	return vm.getCurrModule()->getVars()->get(as<VarStr>(args[1])->get()) != nullptr
	       ? vm.getTrue()
	       : vm.getFalse();
}

Var *setMaxCallstacks(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		      const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc,
			"expected int argument for max count, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	vm.setMaxRecurseCount(mpz_get_ui(as<VarInt>(args[1])->get()));
	return vm.getNil();
}

Var *getMaxCallstacks(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		      const Map<String, AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, vm.getMaxRecurseCount());
}

INIT_MODULE(Sys)
{
	VarModule *mod = vm.getCurrModule();

	mod->addNativeFn("exitNative", _exit, 1);
	mod->addNativeFn("varExists", varExists, 1);
	mod->addNativeFn("setMaxCallstacksNative", setMaxCallstacks, 1);
	mod->addNativeFn("getMaxCallstacks", getMaxCallstacks, 0);

	mod->addNativeVar("args", vm.getCLIArgs());

	mod->addNativeVar("installPrefix", vm.makeVar<VarStr>(loc, STRINGIFY(INSTALL_PREFIX)));

	mod->addNativeVar("selfBin", vm.makeVar<VarStr>(loc, vm.getSelfBin()));
	mod->addNativeVar("selfBase", vm.makeVar<VarStr>(loc, vm.getSelfBase()));
	mod->addNativeVar("mainModulePath", vm.makeVar<VarStr>(loc, vm.getMainModulePath()));

	mod->addNativeVar("versionMajor", vm.makeVar<VarInt>(loc, VERSION_MAJOR));
	mod->addNativeVar("versionMinor", vm.makeVar<VarInt>(loc, VERSION_MINOR));
	mod->addNativeVar("versionPatch", vm.makeVar<VarInt>(loc, VERSION_PATCH));

	mod->addNativeVar("buildDate", vm.makeVar<VarStr>(loc, BUILD_DATE));
	mod->addNativeVar("buildCompiler", vm.makeVar<VarStr>(loc, BUILD_CXX_COMPILER));

	mod->addNativeVar("DEFAULT_MAX_CALLSTACKS", vm.makeVar<VarInt>(loc, MAX_RECURSE_COUNT));
	return true;
}