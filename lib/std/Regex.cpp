#include "RegexType.hpp"

namespace fer
{

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *regexNew(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected regex to be a string, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarInt>()) {
		vm.fail(loc,
			"expected regex flags mask to be an int, found: ", vm.getTypeName(args[2]));
		return nullptr;
	}
	return vm.makeVar<VarRegex>(loc, as<VarStr>(args[1])->get(),
				    (std::regex::flag_type)as<VarInt>(args[2])->get());
}

Var *regexMatch(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected regex target to be a string, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarStr>() && !args[2]->is<VarVec>() && !args[2]->is<VarNil>()) {
		vm.fail(loc,
			"expected regex capture destination "
			"to be a string, vector, or nil, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[3]->is<VarBool>()) {
		vm.fail(
		loc, "expected ignore first match to be a bool, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	StringRef target = as<VarStr>(args[1])->get();
	Var *matches	 = nullptr;
	if(args[2]->is<VarVec>() || args[2]->is<VarStr>()) matches = args[2];
	bool ignoreMatch = as<VarBool>(args[3])->get();
	return as<VarRegex>(args[0])->match(target, loc, matches, ignoreMatch) ? vm.getTrue()
									       : vm.getFalse();
}

INIT_MODULE(Regex)
{
	VarModule *mod = vm.getCurrModule();

	vm.registerType<VarRegex>(loc, "Regex");

	vm.addNativeTypeFn<VarRegex>(loc, "matchNative", regexMatch, 3);

	mod->addNativeFn("newNative", regexNew, 2, false);

	mod->addNativeVar("icase", vm.makeVar<VarInt>(loc, std::regex_constants::icase));
	mod->addNativeVar("nosubs", vm.makeVar<VarInt>(loc, std::regex_constants::nosubs));
	mod->addNativeVar("optimize", vm.makeVar<VarInt>(loc, std::regex_constants::optimize));
	mod->addNativeVar("collate", vm.makeVar<VarInt>(loc, std::regex_constants::collate));
	mod->addNativeVar("ecmascript", vm.makeVar<VarInt>(loc, std::regex_constants::ECMAScript));
	mod->addNativeVar("basic", vm.makeVar<VarInt>(loc, std::regex_constants::basic));
	mod->addNativeVar("extended", vm.makeVar<VarInt>(loc, std::regex_constants::extended));
	mod->addNativeVar("awk", vm.makeVar<VarInt>(loc, std::regex_constants::awk));
	mod->addNativeVar("grep", vm.makeVar<VarInt>(loc, std::regex_constants::grep));
	mod->addNativeVar("egrep", vm.makeVar<VarInt>(loc, std::regex_constants::egrep));
	// Not sure why but multiline is undefined on Windows MSVC at the moment.
#if !defined(FER_OS_WINDOWS) && (__cplusplus >= 201703L || !defined __STRICT_ANSI__)
	mod->addNativeVar("multiline", vm.makeVar<VarInt>(loc, std::regex_constants::multiline));
#endif
	return true;
}

} // namespace fer