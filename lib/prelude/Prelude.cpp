#include <cmath> // std::round()
#include <fcntl.h>
#include <filesystem> // used by File.hpp.in

#include "FS.hpp" // used by File.hpp.in
#include "VM/Interpreter.hpp"

// These headers are below the Feral headers (above), because FER_OS_WINDOWS is defined in them.
#if defined(FER_OS_WINDOWS)
#include <io.h>
#else
#include <dirent.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fer
{

#include "Incs/Bool.hpp.in"
#include "Incs/Bytebuffer.hpp.in"
#include "Incs/Enum.hpp.in"
#include "Incs/File.hpp.in"
#include "Incs/Flt.hpp.in"
#include "Incs/Int.hpp.in"
#include "Incs/Map.hpp.in"
#include "Incs/Nil.hpp.in"
#include "Incs/Str.hpp.in"
#include "Incs/Struct.hpp.in"
#include "Incs/TypeID.hpp.in"
#include "Incs/Vec.hpp.in"

// Converters

#include "Incs/ToBool.hpp.in"
#include "Incs/ToFlt.hpp.in"
#include "Incs/ToInt.hpp.in"
#include "Incs/ToStr.hpp.in"

Var *allGetTypeID(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarTypeID>(loc, args[0]->getType());
}

Var *allGetTypeFnID(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
		    const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarTypeID>(loc, args[0]->getTypeFnID());
}

Var *allGetTypeStr(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
		   const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarStr>(loc, vm.getTypeName(args[0]));
}

Var *allEq(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
	   const StringMap<AssnArgData> &assn_args)
{
	return args[0]->getType() == args[1]->getType() ? vm.getTrue() : vm.getFalse();
}

Var *allNe(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
	   const StringMap<AssnArgData> &assn_args)
{
	return args[0]->getType() != args[1]->getType() ? vm.getTrue() : vm.getFalse();
}

Var *allNilCoalesce(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
		    const StringMap<AssnArgData> &assn_args)
{
	return !args[0]->is<VarNil>() ? args[0] : args[1];
}

Var *allSetThreadSafe(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
		      const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarBool>()) {
		vm.fail(loc, "expected parameter for setThreadSafe to be bool, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	args[0]->setThreadSafe(as<VarBool>(args[1])->getVal());
	return vm.getNil();
}

Var *allCopy(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
	     const StringMap<AssnArgData> &assn_args)
{
	Var *copy = vm.copyVar(loc, args[0]);
	// decreased because system internally will increment it again
	return vm.decVarRef(copy, false);
}

// This is useful when a new (struct) instance is created and inserted into a container,
// but must also be returned as a reference and not a copy.
// If a new instance is created and simply returned without storing in a container,
// there is no point in calling this since reference count of that object will be 1
// and hence the VM won't create a copy of it when used in creating a new var.
Var *reference(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
	       const StringMap<AssnArgData> &assn_args)
{
	args[1]->setLoadAsRef();
	return args[1];
}

Var *raise(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
	   const StringMap<AssnArgData> &assn_args)
{
	String res;
	for(size_t i = 1; i < args.size(); ++i) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{args[i]};
		if(!vm.callVarAndExpect<VarStr>(loc, "str", v, tmp, {})) return nullptr;
		res += as<VarStr>(v)->getVal();
		vm.decVarRef(v);
	}
	vm.fail(loc, "raised: ", res);
	return nullptr;
}

Var *evaluateCode(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected argument to be of type string, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	StringRef code = as<VarStr>(args[1])->getVal();
	Var *res       = vm.eval(loc, code, false);
	if(!res) {
		vm.fail(loc, "failed to evaluate code: ", code);
		return nullptr;
	}
	return vm.decVarRef(res, false);
}

Var *evaluateExpr(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected argument to be of type string, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	StringRef expr = as<VarStr>(args[1])->getVal();
	Var *res       = vm.eval(loc, expr, true);
	if(!res) {
		vm.fail(loc, "failed to evaluate expr: ", expr);
		return nullptr;
	}
	return vm.decVarRef(res, false);
}

// getOSName and getOSDistro must be here because I don't want OS module's dependency on FS or
// vice-versa.

Var *getOSName(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
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

Var *getOSDistro(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
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

// Stuff from std/sys module

Var *_exit(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
	   const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc, "expected integer for exit code, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	vm.setExitCalled(true);
	vm.setExitCode(as<VarInt>(args[1])->getVal());
	return vm.getNil();
}

Var *varExists(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
	       const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for variable name, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	VarModule *mod	 = vm.getCurrModule();
	bool providedMod = false;
	if(args.size() > 2 && args[2] && args[2]->is<VarModule>()) {
		mod	    = as<VarModule>(args[2]);
		providedMod = true;
	}

	Vars *moduleVars = mod->getVars();
	StringRef var	 = as<VarStr>(args[1])->getVal();
	return moduleVars->get(var) || (!providedMod && vm.getGlobal(var)) ? vm.getTrue()
									   : vm.getFalse();
}

Var *setMaxCallstacks(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
		      const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc,
			"expected int argument for max count, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	vm.setRecurseMax(as<VarInt>(args[1])->getVal());
	return vm.getNil();
}

Var *getMaxCallstacks(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
		      const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, vm.getRecurseMax());
}

Var *addGlobalModulePaths(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
			  const StringMap<AssnArgData> &assn_args)
{
	for(size_t i = 1; i < args.size(); ++i) {
		auto &arg = args[i];
		if(!arg->is<VarStr>()) {
			vm.fail(loc,
				"expected string argument for path, found: ", vm.getTypeName(arg));
			return nullptr;
		}
	}
	if(!fs::exists(vm.getGlobalModulePathsFile())) {
		FILE *f = fopen(vm.getGlobalModulePathsFile(), "w");
		fclose(f);
	}
	String data;
	Vector<StringRef> existingData;
	if(fs::read(vm.getGlobalModulePathsFile(), data, true)) {
		existingData = utils::stringDelim(data, "\n");
	}
	FILE *f	     = fopen(vm.getGlobalModulePathsFile(), "a+");
	size_t added = 0;
	for(size_t i = 1; i < args.size(); ++i) {
		VarStr *arg = as<VarStr>(args[i]);
		auto exists = std::find(existingData.begin(), existingData.end(), arg->getVal());
		if(exists != existingData.end()) continue;
		fwrite(arg->getVal().data(), sizeof(char), arg->getVal().size(), f);
		fwrite("\n", sizeof(char), 1, f);
		++added;
	}
	fclose(f);
	return vm.makeVar<VarInt>(loc, added);
}

Var *removeGlobalModulePaths(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
			     const StringMap<AssnArgData> &assn_args)
{
	for(size_t i = 1; i < args.size(); ++i) {
		auto &arg = args[i];
		if(!arg->is<VarStr>()) {
			vm.fail(loc,
				"expected string argument for path, found: ", vm.getTypeName(arg));
			return nullptr;
		}
	}
	if(!fs::exists(vm.getGlobalModulePathsFile())) {
		FILE *f = fopen(vm.getGlobalModulePathsFile(), "w");
		fclose(f);
	}
	String data;
	Vector<StringRef> existingData;
	if(fs::read(vm.getGlobalModulePathsFile(), data, true)) {
		existingData = utils::stringDelim(data, "\n");
	}
	size_t removed = 0;
	for(size_t i = 1; i < args.size(); ++i) {
		VarStr *arg = as<VarStr>(args[i]);
		auto exists = std::find(existingData.begin(), existingData.end(), arg->getVal());
		if(exists == existingData.end()) continue;
		existingData.erase(exists);
		++removed;
		--i;
	}
	FILE *f = fopen(vm.getGlobalModulePathsFile(), "w+");
	for(auto &data : existingData) {
		fwrite(data.data(), sizeof(char), data.size(), f);
		fwrite("\n", sizeof(char), 1, f);
	}
	fclose(f);
	return vm.makeVar<VarInt>(loc, removed);
}

INIT_MODULE(Prelude)
{
	VarModule *mod = vm.getCurrModule(); // prelude module

	// global functions
	vm.addNativeFn(loc, "ref", reference, 1);
	vm.addNativeFn(loc, "raise", raise, 1, true);
	vm.addNativeFn(loc, "evalCode", evaluateCode, 1);
	vm.addNativeFn(loc, "evalExpr", evaluateExpr, 1);
	vm.addNativeFn(loc, "getOSName", getOSName, 0);
	vm.addNativeFn(loc, "getOSDistro", getOSDistro, 0);
	// enum/struct
	vm.addNativeFn(loc, "enum", createEnum, 0, true);
	vm.addNativeFn(loc, "struct", createStruct, 0);

	// module functions
	mod->addNativeFn(vm, "addGlobalModulePaths", addGlobalModulePaths, 1, true);
	mod->addNativeFn(vm, "removeGlobalModulePaths", removeGlobalModulePaths, 1, true);

	// VM altering variables
	mod->addNativeVar("moduleDirs", vm.getModuleDirs());
	mod->addNativeVar("moduleFinders", vm.getModuleFinders());

	// fundamental functions for builtin types
	vm.addNativeTypeFn<VarAll>(loc, "_type_", allGetTypeID, 0);
	vm.addNativeTypeFn<VarAll>(loc, "_typefid_", allGetTypeFnID, 0);
	vm.addNativeTypeFn<VarAll>(loc, "_typestr_", allGetTypeStr, 0);
	vm.addNativeTypeFn<VarAll>(loc, "==", allEq, 1);
	vm.addNativeTypeFn<VarAll>(loc, "!=", allNe, 1);
	vm.addNativeTypeFn<VarAll>(loc, "\?\?", allNilCoalesce, 1);
	vm.addNativeTypeFn<VarAll>(loc, "setThreadSafe", allSetThreadSafe, 1);
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

	// int iterator
	vm.addNativeFn(loc, "irange", intRange, 1, true);
	vm.addNativeTypeFn<VarIntIterator>(loc, "next", getIntIteratorNext, 0);

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

	vm.addNativeTypeFn<VarStr>(loc, "len", strSize, 0);
	vm.addNativeTypeFn<VarStr>(loc, "clear", strClear, 0);
	vm.addNativeTypeFn<VarStr>(loc, "empty", strEmpty, 0);
	vm.addNativeTypeFn<VarStr>(loc, "front", strFront, 0);
	vm.addNativeTypeFn<VarStr>(loc, "back", strBack, 0);
	vm.addNativeTypeFn<VarStr>(loc, "push", strPush, 1);
	vm.addNativeTypeFn<VarStr>(loc, "pop", strPop, 0);
	vm.addNativeTypeFn<VarStr>(loc, "isChAt", strIsChAt, 2);
	vm.addNativeTypeFn<VarStr>(loc, "set", strSetAt, 2);
	vm.addNativeTypeFn<VarStr>(loc, "insert", strInsert, 2);
	vm.addNativeTypeFn<VarStr>(loc, "erase", strErase, 1);
	vm.addNativeTypeFn<VarStr>(loc, "find", strFind, 1);
	vm.addNativeTypeFn<VarStr>(loc, "rfind", strRFind, 1);
	vm.addNativeTypeFn<VarStr>(loc, "substrNative", strSubstr, 2);
	vm.addNativeTypeFn<VarStr>(loc, "lastIdx", strLast, 0);
	vm.addNativeTypeFn<VarStr>(loc, "trim", strTrim, 0);
	vm.addNativeTypeFn<VarStr>(loc, "lower", strLower, 0);
	vm.addNativeTypeFn<VarStr>(loc, "upper", strUpper, 0);
	vm.addNativeTypeFn<VarStr>(loc, "replace", strReplace, 2);
	vm.addNativeTypeFn<VarStr>(loc, "splitNative", strSplit, 2);
	vm.addNativeTypeFn<VarStr>(loc, "startsWith", strStartsWith, 1);
	vm.addNativeTypeFn<VarStr>(loc, "endsWith", strEndsWith, 1);
	vm.addNativeTypeFn<VarStr>(loc, "fmt", strFormat, 0, true);
	vm.addNativeTypeFn<VarStr>(loc, "getBinStrFromHexStr", hexStrToBinStr, 0);
	vm.addNativeTypeFn<VarStr>(loc, "getUTF8CharFromBinStr", utf8CharFromBinStr, 0);

	vm.addNativeTypeFn<VarStr>(loc, "byt", byt, 0);
	vm.addNativeTypeFn<VarInt>(loc, "chr", chr, 0);

	// vec
	mod->addNativeFn(vm, "vecNew", vecNew, 0, true);

	vm.addNativeTypeFn<VarVec>(loc, "len", vecSize, 0);
	vm.addNativeTypeFn<VarVec>(loc, "capacity", vecCapacity, 0);
	vm.addNativeTypeFn<VarVec>(loc, "isRef", vecIsRef, 0);
	vm.addNativeTypeFn<VarVec>(loc, "empty", vecEmpty, 0);
	vm.addNativeTypeFn<VarVec>(loc, "front", vecFront, 0);
	vm.addNativeTypeFn<VarVec>(loc, "back", vecBack, 0);
	vm.addNativeTypeFn<VarVec>(loc, "push", vecPush, 1, true);
	vm.addNativeTypeFn<VarVec>(loc, "pop", vecPop, 0);
	vm.addNativeTypeFn<VarVec>(loc, "clear", vecClear, 0);
	vm.addNativeTypeFn<VarVec>(loc, "erase", vecErase, 1);
	vm.addNativeTypeFn<VarVec>(loc, "insert", vecInsert, 2);
	vm.addNativeTypeFn<VarVec>(loc, "appendNative", vecAppend, 3);
	vm.addNativeTypeFn<VarVec>(loc, "swap", vecSwap, 2);
	vm.addNativeTypeFn<VarVec>(loc, "reverse", vecReverse, 0);
	vm.addNativeTypeFn<VarVec>(loc, "set", vecSetAt, 2);
	vm.addNativeTypeFn<VarVec>(loc, "at", vecAt, 1);
	vm.addNativeTypeFn<VarVec>(loc, "[]", vecAt, 1);

	vm.addNativeTypeFn<VarVec>(loc, "subNative", vecSub, 2);
	vm.addNativeTypeFn<VarVec>(loc, "sliceNative", vecSlice, 2);

	vm.addNativeTypeFn<VarVec>(loc, "each", vecEach, 0);
	vm.addNativeTypeFn<VarVecIterator>(loc, "next", vecIteratorNext, 0);

	// map
	mod->addNativeFn(vm, "mapNew", mapNew, 0, true);
	vm.addNativeTypeFn<VarMap>(loc, "len", mapSize, 0);
	vm.addNativeTypeFn<VarMap>(loc, "isRef", mapIsRef, 0);
	vm.addNativeTypeFn<VarMap>(loc, "empty", mapEmpty, 0);
	vm.addNativeTypeFn<VarMap>(loc, "insert", mapInsert, 2);
	vm.addNativeTypeFn<VarMap>(loc, "erase", mapErase, 1);
	vm.addNativeTypeFn<VarMap>(loc, "clear", mapClear, 0);
	vm.addNativeTypeFn<VarMap>(loc, "find", mapFind, 1);
	vm.addNativeTypeFn<VarMap>(loc, "at", mapAt, 1);
	vm.addNativeTypeFn<VarMap>(loc, "[]", mapAt, 1);

	vm.addNativeTypeFn<VarMap>(loc, "each", mapEach, 0);
	vm.addNativeTypeFn<VarMapIterator>(loc, "next", mapIteratorNext, 0);

	// struct
	vm.addNativeTypeFn<VarStructDef>(loc, "setTypeName", structDefSetTypeName, 1);
	vm.addNativeTypeFn<VarStructDef>(loc, "getFields", structDefGetFields, 0);
	vm.addNativeTypeFn<VarStructDef>(loc, "[]", structDefGetFieldValue, 1);
	vm.addNativeTypeFn<VarStructDef>(loc, "len", structDefLen, 0);

	vm.addNativeTypeFn<VarStruct>(loc, "getFields", structGetFields, 0);
	vm.addNativeTypeFn<VarStruct>(loc, "setField", structSetFieldValue, 2);
	vm.addNativeTypeFn<VarStruct>(loc, "str", structToStr, 0);
	vm.addNativeTypeFn<VarStruct>(loc, "len", structLen, 0);

	// bytebuffer
	mod->addNativeFn(vm, "bytebufferNew", bytebufferNewNative, 1);

	vm.addNativeTypeFn<VarBytebuffer>(loc, "len", bytebufferLen, 0);
	vm.addNativeTypeFn<VarBytebuffer>(loc, "capacity", bytebufferCapacity, 0);
	vm.addNativeTypeFn<VarBytebuffer>(loc, "str", bytebufferToStr, 0);

	// file/filesystem
	mod->addNativeFn(vm, "fsFopen", fsOpen, 3);
	mod->addNativeFn(vm, "fsWalkDir", fsWalkDir, 3);
	mod->addNativeFn(vm, "fsAbsPath", fsAbsPath, 1);
	// file descriptor
	mod->addNativeFn(vm, "fsFdOpen", fdOpen, 2);
	mod->addNativeFn(vm, "fsFdRead", fdRead, 2);
	mod->addNativeFn(vm, "fsFdWrite", fdWrite, 2);
	mod->addNativeFn(vm, "fsFdClose", fdClose, 1);
	// files and dirs
	mod->addNativeFn(vm, "fsExists", fsExists, 1);
	mod->addNativeFn(vm, "fsInstall", fsInstall, 2);
	mod->addNativeFn(vm, "fsMklink", fsMklink, 2);
	mod->addNativeFn(vm, "fsMove", fsMove, 2);
	mod->addNativeFn(vm, "fsMkdir", fsMkdir, 1, true);
	mod->addNativeFn(vm, "fsRemove", fsRemove, 1, true);
	mod->addNativeFn(vm, "fsCopy", fsCopy, 2, true);

	vm.addNativeTypeFn<VarFile>(loc, "reopenNative", fileReopen, 2);
	vm.addNativeTypeFn<VarFile>(loc, "lines", fileLines, 0);
	vm.addNativeTypeFn<VarFile>(loc, "seek", fileSeek, 2);
	vm.addNativeTypeFn<VarFile>(loc, "eachLine", fileEachLine, 0);
	vm.addNativeTypeFn<VarFile>(loc, "readAll", fileReadAll, 0);
	vm.addNativeTypeFn<VarFile>(loc, "readBlocks", fileReadBlocks, 2);

	vm.addNativeTypeFn<VarFileIterator>(loc, "next", fileIteratorNext, 0);

	// constants (for file/filesystem)
	// stdin, stdout, stderr file descriptors
	mod->addNativeVar("fsStdin", vm.makeVar<VarInt>(loc, STDIN_FILENO));
	mod->addNativeVar("fsStdout", vm.makeVar<VarInt>(loc, STDOUT_FILENO));
	mod->addNativeVar("fsStderr", vm.makeVar<VarInt>(loc, STDERR_FILENO));
	// fs.walkdir()
	mod->addNativeVar("FS_WALK_FILES", vm.makeVar<VarInt>(loc, WalkEntry::FILES));
	mod->addNativeVar("FS_WALK_DIRS", vm.makeVar<VarInt>(loc, WalkEntry::DIRS));
	mod->addNativeVar("FS_WALK_RECURSE", vm.makeVar<VarInt>(loc, WalkEntry::RECURSE));
	// <file>.seek()
	mod->addNativeVar("FS_SEEK_SET", vm.makeVar<VarInt>(loc, SEEK_SET));
	mod->addNativeVar("FS_SEEK_CUR", vm.makeVar<VarInt>(loc, SEEK_CUR));
	mod->addNativeVar("FS_SEEK_END", vm.makeVar<VarInt>(loc, SEEK_END));
	// file descriptor flags
	mod->addNativeVar("FS_O_RDONLY", vm.makeVar<VarInt>(loc, O_RDONLY));
	mod->addNativeVar("FS_O_WRONLY", vm.makeVar<VarInt>(loc, O_WRONLY));
	mod->addNativeVar("FS_O_RDWR", vm.makeVar<VarInt>(loc, O_RDWR));
	mod->addNativeVar("FS_O_APPEND", vm.makeVar<VarInt>(loc, O_APPEND));
	mod->addNativeVar("FS_O_CREAT", vm.makeVar<VarInt>(loc, O_CREAT));
#if defined(FER_OS_LINUX) || defined(FER_OS_APPLE)
	mod->addNativeVar("FS_O_DSYNC", vm.makeVar<VarInt>(loc, O_DSYNC));
#endif
	mod->addNativeVar("FS_O_EXCL", vm.makeVar<VarInt>(loc, O_EXCL));
#if !defined(FER_OS_WINDOWS)
	mod->addNativeVar("FS_O_NOCTTY", vm.makeVar<VarInt>(loc, O_NOCTTY));
	mod->addNativeVar("FS_O_NONBLOCK", vm.makeVar<VarInt>(loc, O_NONBLOCK));
	mod->addNativeVar("FS_O_SYNC", vm.makeVar<VarInt>(loc, O_SYNC));
#endif
#if defined(FER_OS_LINUX)
	mod->addNativeVar("FS_O_RSYNC", vm.makeVar<VarInt>(loc, O_RSYNC));
#endif
	mod->addNativeVar("FS_O_TRUNC", vm.makeVar<VarInt>(loc, O_TRUNC));

	// From std/sys

	mod->addNativeFn(vm, "exitNative", _exit, 1);
	// va because there can be no proxy for this function (to make args[2] (VarModule) optional)
	mod->addNativeFn(vm, "varExists", varExists, 1, true);
	mod->addNativeFn(vm, "setMaxCallstacksNative", setMaxCallstacks, 1);
	mod->addNativeFn(vm, "getMaxCallstacks", getMaxCallstacks, 0);

	mod->addNativeVar("args", vm.getCLIArgs());

	mod->addNativeVar("binaryPath", vm.makeVar<VarStr>(loc, vm.getBinaryPath()));

	mod->addNativeVar("versionMajor", vm.makeVar<VarInt>(loc, PROJECT_MAJOR));
	mod->addNativeVar("versionMinor", vm.makeVar<VarInt>(loc, PROJECT_MINOR));
	mod->addNativeVar("versionPatch", vm.makeVar<VarInt>(loc, PROJECT_PATCH));

	mod->addNativeVar("buildDate", vm.makeVar<VarStr>(loc, BUILD_DATE));
	mod->addNativeVar("buildCompiler", vm.makeVar<VarStr>(loc, BUILD_COMPILER));
	mod->addNativeVar("buildType", vm.makeVar<VarStr>(loc, CMAKE_BUILD_TYPE));
	mod->addNativeVar("minCmakeVersion", vm.makeVar<VarStr>(loc, MIN_CMAKE_VERSION));
	mod->addNativeVar("cmakeVersion", vm.makeVar<VarStr>(loc, CMAKE_VERSION));

	mod->addNativeVar("installPath", vm.makeVar<VarStr>(loc, INSTALL_PATH));
	mod->addNativeVar("tempPath", vm.makeVar<VarStr>(loc, TEMP_PATH));

	mod->addNativeVar("DEFAULT_MAX_CALLSTACKS",
			  vm.makeVar<VarInt>(loc, DEFAULT_MAX_RECURSE_COUNT));

	return true;
}

} // namespace fer