#include <cmath> // std::round()

#include "Utils.hpp"
#include "VM/Interpreter.hpp"

namespace fer
{

#include "Core/Bool.hpp.in"
#include "Core/Flt.hpp.in"
#include "Core/Int.hpp.in"
#include "Core/Nil.hpp.in"
#include "Core/Str.hpp.in"
#include "Core/ToBool.hpp.in"
#include "Core/ToFlt.hpp.in"
#include "Core/ToInt.hpp.in"
#include "Core/ToStr.hpp.in"
#include "Core/TypeID.hpp.in"

Var *allGetTypeID(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarTypeID>(loc, args[0]->getType());
}

Var *allGetTypeFnID(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		    const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarTypeID>(loc, args[0]->getTypeFnID());
}

Var *allGetTypeStr(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		   const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarStr>(loc, vm.getTypeName(args[0]));
}

Var *allEq(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	   const StringMap<AssnArgData> &assn_args)
{
	return args[0]->getType() == args[1]->getType() ? vm.getTrue() : vm.getFalse();
}

Var *allNe(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	   const StringMap<AssnArgData> &assn_args)
{
	return args[0]->getType() != args[1]->getType() ? vm.getTrue() : vm.getFalse();
}

Var *allCopy(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const StringMap<AssnArgData> &assn_args)
{
	Var *copy = args[0]->copy(loc);
	// decreased because system internally will increment it again
	copy->dref();
	return copy;
}

// This is useful when a new (struct) instance is created and inserted into a container,
// but must also be returned as a reference and not a copy.
// If a new instance is created and simply returned without storing in a container,
// there is no point in calling this since reference count of that object will be 1
// and hence the VM won't create a copy of it when used in creating a new var.
Var *reference(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const StringMap<AssnArgData> &assn_args)
{
	args[1]->setLoadAsRef();
	return args[1];
}

Var *raise(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	   const StringMap<AssnArgData> &assn_args)
{
	String res;
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
		res += as<VarStr>(v)->get();
		decref(v);
	}
	vm.fail(loc, "raised: ", res);
	return nullptr;
}

Var *loadModule(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected argument to be of type string, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!vm.loadNativeModule(loc, as<VarStr>(args[1])->get())) {
		vm.fail(loc, "failed to load module: ", as<VarStr>(args[1])->get(),
			"; look at the error above");
		return nullptr;
	}
	return vm.getNil();
}

Var *importFile(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected argument to be of type string, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	String file = as<VarStr>(args[1])->get();
	if(!vm.findImport(file)) {
		vm.fail(args[1]->getLoc(), "import: ", file, vm.getFeralImportExtension(),
			" not found in locs: ", vecToStr(vm.getImportDirs()));
		return nullptr;
	}
	if(!vm.hasModule(file)) {
		int res = vm.compileAndRun(loc, file);
		if(res != 0) {
			vm.fail(args[1]->getLoc(),
				"module import failed, look at error above (exit code: ", res, ")");
			return nullptr;
		}
	}
	return vm.getModule(file);
}

Var *evaluateCode(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected argument to be of type string, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	StringRef code = as<VarStr>(args[1])->get();
	Var *res       = vm.eval(loc, code, false);
	if(!res) {
		vm.fail(loc, "failed to evaluate code: ", code);
		return nullptr;
	}
	res->dref();
	return res;
}

Var *evaluateExpr(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected argument to be of type string, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	StringRef expr = as<VarStr>(args[1])->get();
	Var *res       = vm.eval(loc, expr, true);
	if(!res) {
		vm.fail(loc, "failed to evaluate expr: ", expr);
		return nullptr;
	}
	res->dref();
	return res;
}

// getOSName and getOSDistro must be here because I don't want OS module's dependency on FS or
// vice-versa.

Var *getOSName(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const StringMap<AssnArgData> &assn_args)
{
	String name;
#if defined(FER_OS_WINDOWS)
	name = "windows";
#elif defined(FER_OS_LINUX)
	name = "linux";
#elif defined(FER_OS_ANDROID)
	name = "android";
#elif defined(FER_OS_BSD)
	name = "bsd";
#elif defined(FER_OS_APPLE)
	name = "macos";
#endif
	return vm.makeVar<VarStr>(loc, name);
}

Var *getOSDistro(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		 const StringMap<AssnArgData> &assn_args)
{
	String distro;
#if defined(FER_OS_WINDOWS)
#if defined(FER_OS_WINDOWS64)
	distro = "windows64";
#else
	distro = "windows";
#endif
#elif defined(FER_OS_LINUX)
	distro = "linux"; // arch,ubuntu,etc.
#elif defined(FER_OS_ANDROID)
	distro = "android"; // version name - lollipop, marshmellow, etc.
#elif defined(FER_OS_BSD)
#if defined(FER_OS_FREEBSD)
	distro = "freebsd";
#elif defined(FER_OS_NETBSD)
	distro = "netbsd";
#elif defined(FER_OS_OPENBSD)
	distro = "openbsd";
#elif defined(FER_OS_BSDI)
	distro = "bsdi";
#elif defined(FER_OS_DRAGONFLYBSD)
	distro = "dragonflybsd";
#else
	distro = "bsd";
#endif
#elif defined(FER_OS_APPLE)
	distro = "macos";
#endif
	return vm.makeVar<VarStr>(loc, distro);
}

Var *isMainModule(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	return vm.getCurrModule()->getMod()->isMainModule() ? vm.getTrue() : vm.getFalse();
}

INIT_MODULE(Core)
{
	// global functions
	vm.addNativeFn(loc, "ref", reference, 1);
	vm.addNativeFn(loc, "raise", raise, 1, true);
	vm.addNativeFn(loc, "mload", loadModule, 1);
	vm.addNativeFn(loc, "import", importFile, 1);
	vm.addNativeFn(loc, "evalCode", evaluateCode, 1);
	vm.addNativeFn(loc, "evalExpr", evaluateExpr, 1);
	vm.addNativeFn(loc, "getOSName", getOSName, 0);
	vm.addNativeFn(loc, "getOSDistro", getOSDistro, 0);
	vm.addNativeFn(loc, "_isMainModule_", isMainModule, 1);

	// fundamental functions for builtin types
	vm.addNativeTypeFn<VarAll>(loc, "_type_", allGetTypeID, 0);
	vm.addNativeTypeFn<VarAll>(loc, "_typefid_", allGetTypeFnID, 0);
	vm.addNativeTypeFn<VarAll>(loc, "_typestr_", allGetTypeStr, 0);
	vm.addNativeTypeFn<VarAll>(loc, "==", allEq, 1);
	vm.addNativeTypeFn<VarAll>(loc, "!=", allNe, 1);
	vm.addNativeTypeFn<VarAll>(loc, "copy", allCopy, 0);

	// to bool
	vm.addNativeTypeFn<VarAll>(loc, "bool", allToBool, 0);
	vm.addNativeTypeFn<VarNil>(loc, "bool", nilToBool, 0);
	vm.addNativeTypeFn<VarBool>(loc, "bool", boolToBool, 0);
	vm.addNativeTypeFn<VarInt>(loc, "bool", intToBool, 0);
	vm.addNativeTypeFn<VarFlt>(loc, "bool", fltToBool, 0);
	vm.addNativeTypeFn<VarStr>(loc, "bool", strToBool, 0);
	vm.addNativeTypeFn<VarVec>(loc, "bool", vecToBool, 0);
	vm.addNativeTypeFn<VarMap>(loc, "bool", mapToBool, 0);
	vm.addNativeTypeFn<VarTypeID>(loc, "bool", typeIDToBool, 0);

	// to int
	vm.addNativeTypeFn<VarNil>(loc, "int", nilToInt, 0);
	vm.addNativeTypeFn<VarBool>(loc, "int", boolToInt, 0);
	vm.addNativeTypeFn<VarInt>(loc, "int", intToInt, 0);
	vm.addNativeTypeFn<VarFlt>(loc, "int", fltToInt, 0);
	vm.addNativeTypeFn<VarStr>(loc, "int", strToInt, 0);
	vm.addNativeTypeFn<VarTypeID>(loc, "int", typeIDToInt, 0);

	// to float
	vm.addNativeTypeFn<VarNil>(loc, "flt", nilToFlt, 0);
	vm.addNativeTypeFn<VarBool>(loc, "flt", boolToFlt, 0);
	vm.addNativeTypeFn<VarInt>(loc, "flt", intToFlt, 0);
	vm.addNativeTypeFn<VarFlt>(loc, "flt", fltToFlt, 0);
	vm.addNativeTypeFn<VarStr>(loc, "flt", strToFlt, 0);

	// to string
	vm.addNativeTypeFn<VarAll>(loc, "str", allToStr, 0);
	vm.addNativeTypeFn<VarNil>(loc, "str", nilToStr, 0);
	vm.addNativeTypeFn<VarBool>(loc, "str", boolToStr, 0);
	vm.addNativeTypeFn<VarInt>(loc, "str", intToStr, 0);
	vm.addNativeTypeFn<VarFlt>(loc, "str", fltToStr, 0);
	vm.addNativeTypeFn<VarStr>(loc, "str", strToStr, 0);
	vm.addNativeTypeFn<VarTypeID>(loc, "str", typeIDToStr, 0);

	// core type functions

	// nil
	vm.addNativeTypeFn<VarNil>(loc, "==", nilEq, 1);
	vm.addNativeTypeFn<VarNil>(loc, "!=", nilNe, 1);

	// typeID
	vm.addNativeTypeFn<VarTypeID>(loc, "==", typeIDEq, 1);
	vm.addNativeTypeFn<VarTypeID>(loc, "!=", typeIDNe, 1);

	// bool
	vm.addNativeTypeFn<VarBool>(loc, "<", boolLT, 1);
	vm.addNativeTypeFn<VarBool>(loc, ">", boolGT, 1);
	vm.addNativeTypeFn<VarBool>(loc, "<=", boolLE, 1);
	vm.addNativeTypeFn<VarBool>(loc, ">=", boolGE, 1);
	vm.addNativeTypeFn<VarBool>(loc, "==", boolEq, 1);
	vm.addNativeTypeFn<VarBool>(loc, "!=", boolNe, 1);

	vm.addNativeTypeFn<VarBool>(loc, "!", boolNot, 0);

	// int
	vm.addNativeTypeFn<VarInt>(loc, "+", intAdd, 1);
	vm.addNativeTypeFn<VarInt>(loc, "-", intSub, 1);
	vm.addNativeTypeFn<VarInt>(loc, "*", intMul, 1);
	vm.addNativeTypeFn<VarInt>(loc, "/", intDiv, 1);
	vm.addNativeTypeFn<VarInt>(loc, "%", intMod, 1);
	vm.addNativeTypeFn<VarInt>(loc, "<<", intLShift, 1);
	vm.addNativeTypeFn<VarInt>(loc, ">>", intRShift, 1);

	vm.addNativeTypeFn<VarInt>(loc, "+=", intAssnAdd, 1);
	vm.addNativeTypeFn<VarInt>(loc, "-=", intAssnSub, 1);
	vm.addNativeTypeFn<VarInt>(loc, "*=", intAssnMul, 1);
	vm.addNativeTypeFn<VarInt>(loc, "/=", intAssnDiv, 1);
	vm.addNativeTypeFn<VarInt>(loc, "%=", intAssnMod, 1);
	vm.addNativeTypeFn<VarInt>(loc, "<<=", intLShiftAssn, 1);
	vm.addNativeTypeFn<VarInt>(loc, ">>=", intRShiftAssn, 1);

	vm.addNativeTypeFn<VarInt>(loc, "**", intPow, 1);
	vm.addNativeTypeFn<VarInt>(loc, "++x", intPreInc, 0);
	vm.addNativeTypeFn<VarInt>(loc, "x++", intPostInc, 0);
	vm.addNativeTypeFn<VarInt>(loc, "--x", intPreDec, 0);
	vm.addNativeTypeFn<VarInt>(loc, "x--", intPostDec, 0);

	vm.addNativeTypeFn<VarInt>(loc, "u-", intUSub, 0);

	vm.addNativeTypeFn<VarInt>(loc, "<", intLT, 1);
	vm.addNativeTypeFn<VarInt>(loc, ">", intGT, 1);
	vm.addNativeTypeFn<VarInt>(loc, "<=", intLE, 1);
	vm.addNativeTypeFn<VarInt>(loc, ">=", intGE, 1);
	vm.addNativeTypeFn<VarInt>(loc, "==", intEQ, 1);
	vm.addNativeTypeFn<VarInt>(loc, "!=", intNE, 1);

	vm.addNativeTypeFn<VarInt>(loc, "&", intBAnd, 1);
	vm.addNativeTypeFn<VarInt>(loc, "|", intBOr, 1);
	vm.addNativeTypeFn<VarInt>(loc, "^", intBXOr, 1);
	vm.addNativeTypeFn<VarInt>(loc, "~", intBNot, 0);

	vm.addNativeTypeFn<VarInt>(loc, "&=", intAssnBAnd, 1);
	vm.addNativeTypeFn<VarInt>(loc, "|=", intAssnBOr, 1);
	vm.addNativeTypeFn<VarInt>(loc, "^=", intAssnBXOr, 1);

	vm.addNativeTypeFn<VarInt>(loc, "sqrt", intSqRoot, 0);
	vm.addNativeTypeFn<VarInt>(loc, "popcnt", intPopCnt, 0);

	// flt
	vm.addNativeTypeFn<VarFlt>(loc, "+", fltAdd, 1);
	vm.addNativeTypeFn<VarFlt>(loc, "-", fltSub, 1);
	vm.addNativeTypeFn<VarFlt>(loc, "*", fltMul, 1);
	vm.addNativeTypeFn<VarFlt>(loc, "/", fltDiv, 1);

	vm.addNativeTypeFn<VarFlt>(loc, "+=", fltAssnAdd, 1);
	vm.addNativeTypeFn<VarFlt>(loc, "-=", fltAssnSub, 1);
	vm.addNativeTypeFn<VarFlt>(loc, "*=", fltAssnMul, 1);
	vm.addNativeTypeFn<VarFlt>(loc, "/=", fltAssnDiv, 1);

	vm.addNativeTypeFn<VarFlt>(loc, "++x", fltPreInc, 0);
	vm.addNativeTypeFn<VarFlt>(loc, "x++", fltPostInc, 0);
	vm.addNativeTypeFn<VarFlt>(loc, "--x", fltPreDec, 0);
	vm.addNativeTypeFn<VarFlt>(loc, "x--", fltPostDec, 0);

	vm.addNativeTypeFn<VarFlt>(loc, "u-", fltUSub, 0);

	vm.addNativeTypeFn<VarFlt>(loc, "**", fltPow, 1);

	vm.addNativeTypeFn<VarFlt>(loc, "<", fltLT, 1);
	vm.addNativeTypeFn<VarFlt>(loc, ">", fltGT, 1);
	vm.addNativeTypeFn<VarFlt>(loc, "<=", fltLE, 1);
	vm.addNativeTypeFn<VarFlt>(loc, ">=", fltGE, 1);
	vm.addNativeTypeFn<VarFlt>(loc, "==", fltEQ, 1);
	vm.addNativeTypeFn<VarFlt>(loc, "!=", fltNE, 1);

	vm.addNativeTypeFn<VarFlt>(loc, "round", fltRound, 0);
	vm.addNativeTypeFn<VarFlt>(loc, "sqrt", fltSqRoot, 0);

	// string
	vm.addNativeTypeFn<VarStr>(loc, "+", strAdd, 1);
	vm.addNativeTypeFn<VarStr>(loc, "*", strMul, 1);

	vm.addNativeTypeFn<VarStr>(loc, "+=", strAddAssn, 1);
	vm.addNativeTypeFn<VarStr>(loc, "*=", strMulAssn, 1);

	vm.addNativeTypeFn<VarStr>(loc, "<", strLT, 1);
	vm.addNativeTypeFn<VarStr>(loc, ">", strGT, 1);
	vm.addNativeTypeFn<VarStr>(loc, "<=", strLE, 1);
	vm.addNativeTypeFn<VarStr>(loc, ">=", strGE, 1);
	vm.addNativeTypeFn<VarStr>(loc, "==", strEq, 1);
	vm.addNativeTypeFn<VarStr>(loc, "!=", strNe, 1);

	vm.addNativeTypeFn<VarStr>(loc, "at", strAt, 1);
	vm.addNativeTypeFn<VarStr>(loc, "[]", strAt, 1);

	return true;
}

} // namespace fer