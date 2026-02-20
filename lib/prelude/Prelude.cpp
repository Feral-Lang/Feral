#include "VM/VM.hpp"

// These headers are below the Feral headers (above), because CORE_OS_WINDOWS is defined in them.
#if defined(CORE_OS_WINDOWS)
#include <io.h>
#else
#include <dirent.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "Incs/Bool.hpp.in"
#include "Incs/Bytebuffer.hpp.in"
#include "Incs/Enum.hpp.in"
#include "Incs/File.hpp.in"
#include "Incs/Flt.hpp.in"
#include "Incs/Int.hpp.in"
#include "Incs/Map.hpp.in"
#include "Incs/Module.hpp.in"
#include "Incs/Nil.hpp.in"
#include "Incs/Path.hpp.in"
#include "Incs/Str.hpp.in"
#include "Incs/Struct.hpp.in"
#include "Incs/TypeID.hpp.in"
#include "Incs/Vec.hpp.in"

// Converters

#include "Incs/ToBool.hpp.in"
#include "Incs/ToFlt.hpp.in"
#include "Incs/ToInt.hpp.in"
#include "Incs/ToStr.hpp.in"

namespace fer
{

FERAL_FUNC(crash, 0, false,
           "  fn() -> Nil\n"
           "Causes the Feral binary to crash. Useful to get VM (C++) callstacks.")
{
    volatile int *tmp = nullptr;
    *tmp              = 5;
    return vm.getNil();
}

FERAL_FUNC(allGetDoc, 0, false,
           "  var.fn() -> Str | Nil\n"
           "Returns the doc string for `var` "
           "(can be function) if one is defined, `nil` otherwise.")
{
    if(!args[1]->hasDoc()) return vm.getNil();
    return args[1]->getDoc();
}

FERAL_FUNC(allSetDoc, 1, false,
           "  var.fn(docStr) -> Nil\n"
           "Sets the doc string for `var` as `docStr`. If `docStr` is `nil`, empties out the doc "
           "string for `var`.")
{
    EXPECT2(VarStr, VarNil, args[1], "doc string");
    auto &mem = vm.getMemoryManager();
    if(args[1]->is<VarNil>()) {
        args[0]->setDoc(vm, nullptr);
    } else {
        VarStr *cp = as<VarStr>(vm.copyVar(loc, args[1]));
        if(!cp) return nullptr;
        args[0]->setDoc(vm, cp);
    }
    return vm.getNil();
}

FERAL_FUNC(allHasAttr, 1, false,
           "  var.fn(name) -> Var\n"
           "Given the attribute `name`, returns the respective value contained in `var`.\n"
           "Requires `var` to be attribute based (like Module / Struct).")
{
    EXPECT_ATTR_BASED(args[0], "var");
    EXPECT(VarStr, args[1], "doc string");
    Var *in        = args[0];
    StringRef attr = as<VarStr>(args[1])->getVal();
    return in->existsAttr(attr) ? vm.getTrue() : vm.getFalse();
}

FERAL_FUNC(allGetAttr, 1, false,
           "  var.fn(name) -> Var\n"
           "Given the attribute `name`, returns the respective value contained in `var`.\n"
           "Requires `var` to be attribute based (like Module / Struct).")
{
    EXPECT_ATTR_BASED(args[0], "var");
    EXPECT(VarStr, args[1], "doc string");
    Var *in        = args[0];
    StringRef attr = as<VarStr>(args[1])->getVal();
    Var *res       = in->getAttr(attr);
    if(!res) {
        vm.fail(loc, "attribute `", attr, "` not found");
        return nullptr;
    }
    return res;
}

FERAL_FUNC(allGetAttrs, 0, false,
           "  var.fn() -> Var\n"
           "Returns all the attribute names contained in `var`.\n"
           "Requires `var` to be attribute based (like Module / Struct).")
{
    EXPECT_ATTR_BASED(args[0], "var");
    Var *in     = args[0];
    VarVec *res = vm.makeVar<VarVec>(loc, in->getAttrCount(), true);
    in->getAttrList(vm, res);
    return res;
}

FERAL_FUNC(allSetAttr, 2, false,
           "  var.fn(name, value) -> value\n"
           "Given the attribute `name`, sets the associated `value` in container `var`.\n"
           "Requires `var` to be attribute based (like Module / Struct).\n"
           "Returns the provided `value`.")
{
    EXPECT_ATTR_BASED(args[0], "var");
    EXPECT(VarStr, args[1], "doc string");
    Var *in        = args[0];
    StringRef attr = as<VarStr>(args[1])->getVal();
    in->setAttr(vm, attr, args[2], true);
    return args[2];
}

FERAL_FUNC(allGetType, 0, false,
           "  var.fn() -> TypeID\n"
           "Returns the type ID of `var`.")
{
    return vm.makeVar<VarTypeID>(loc, args[0]->getType());
}

FERAL_FUNC(allGetSubType, 0, false,
           "  var.fn() -> TypeID\n"
           "Returns the subtype ID of `var`.\n"
           "For example, a struct instance's definition's type.")
{
    return vm.makeVar<VarTypeID>(loc, args[0]->getSubType());
}

FERAL_FUNC(allIsType, 1, false,
           "  var.fn(type) -> Bool\n"
           "Returns `true` if `var` is a `type`.")
{
    EXPECT(VarTypeID, args[1], "type ID");
    return args[0]->getType() == as<VarTypeID>(args[1])->getVal() ? vm.getTrue() : vm.getFalse();
}

FERAL_FUNC(allIsSubType, 1, false,
           "  var.fn(subType) -> Bool\n"
           "Returns `true` if the subtype of `var` is `subType`.")
{
    EXPECT(VarTypeID, args[1], "type ID");
    return args[0]->getSubType() == as<VarTypeID>(args[1])->getVal() ? vm.getTrue() : vm.getFalse();
}

FERAL_FUNC(allGetTypeName, 0, false,
           "  var.fn() -> Str\n"
           "Returns the name of the type of `var`.")
{
    return vm.makeVar<VarStr>(loc, vm.getTypeName(args[0]));
}

FERAL_FUNC(allEq, 1, false,
           "  var.fn(other) -> bool\n"
           "Checks if the types of `var` and `other` are same.")
{
    return args[0]->getType() == args[1]->getType() ? vm.getTrue() : vm.getFalse();
}

FERAL_FUNC(allNe, 1, false,
           "  var.fn(other) -> bool\n"
           "Checks if the types of `var` and `other` are not same.")
{
    return args[0]->getType() != args[1]->getType() ? vm.getTrue() : vm.getFalse();
}

FERAL_FUNC(allNilCoalesce, 1, false,
           "  var.fn(other) -> this | other\n"
           "If `var` is nil, return `other`, otherwise return `var`.")
{
    return !args[0]->is<VarNil>() ? args[0] : args[1];
}

// This is useful when a new (struct) instance is created and inserted into a container,
// but must also be returned as a reference and not a copy.
// If a new instance is created and simply returned without storing in a container,
// there is no point in calling this since reference count of that object will be 1
// and hence the VM won't create a copy of it when used in creating a new var.
FERAL_FUNC(reference, 1, false,
           "  fn(var) -> var\n"
           "Returns the argument itself, but with loadAsRef internal "
           "variable set as true.\n"
           "This ensures that the next time a new object is created "
           "using `var` as the value, `var` is not copied.")
{
    args[1]->setLoadAsRef();
    return args[1];
}

FERAL_FUNC(unreference, 1, false,
           "  fn(var) -> var\n"
           "Returns the argument itself, but with loadAsRef "
           "internal variable set as false.")
{
    args[1]->unsetLoadAsRef();
    return args[1];
}
FERAL_FUNC(constant, 1, false,
           "  fn(var) -> var\n"
           "Returns the argument itself, but with const internal flag set as `true`.\n"
           "This ensures that the next time a new object is created "
           "using `var` as the value, `var` is not copied.")
{
    args[1]->setConst();
    return args[1];
}

FERAL_FUNC(deconstant, 1, false,
           "  fn(var) -> var\n"
           "Returns the argument itself, but with const internal flag set as `false`.")
{
    args[1]->unsetConst();
    return args[1];
}

FERAL_FUNC(raise, 1, true,
           "  fn(data...) -> var\n"
           "Concatenates data into a string by calling `var.str()` on each data item, "
           "which is then used to raise/throw an error.")
{
    String res;
    for(size_t i = 1; i < args.size(); ++i) {
        Var *v = nullptr;
        Array<Var *, 1> tmp{args[i]};
        if(!vm.callVarAndExpect<VarStr>(loc, "str", v, tmp, {})) return nullptr;
        res += as<VarStr>(v)->getVal();
        vm.decVarRef(v);
    }
    vm.fail(loc, "Raised: ", res);
    return nullptr;
}

FERAL_FUNC(evalCode, 1, false,
           "  fn(code:str) -> var\n"
           "Evaluates the given code and returns the result.")
{
    EXPECT(VarStr, args[1], "code");
    StringRef code = as<VarStr>(args[1])->getVal();
    Var *res       = vm.eval(loc, code, false);
    if(!res) {
        vm.fail(loc, "failed to evaluate code: ", code);
        return nullptr;
    }
    return vm.decVarRef(res, false);
}

FERAL_FUNC(evalExpr, 1, false,
           "  fn(expr:str) -> var\n"
           "Evaluates the given code as an expression and returns the result.\n"
           "Unlike `eval()`, this just expects an expression and doesn't need a return statement.")
{
    EXPECT(VarStr, args[1], "code");
    StringRef expr = as<VarStr>(args[1])->getVal();
    Var *res       = vm.eval(loc, expr, true);
    if(!res) {
        vm.fail(loc, "failed to evaluate expr: ", expr);
        return nullptr;
    }
    return vm.decVarRef(res, false);
}

FERAL_FUNC(getCurrModule, 0, false,
           "  fn() -> Module\n"
           "Returns the current module.")
{
    return vm.getCurrModule();
}

FERAL_FUNC(getOSName, 0, false,
           "  fn() -> str\n"
           "Returns the name of the current operating system, currently one of the following:\n"
           "- windows\n"
           "- linux\n"
           "- android\n"
           "- bsd\n"
           "- macos")
{
    String name;
#if defined(CORE_OS_WINDOWS)
    name = "windows";
#elif defined(CORE_OS_LINUX)
    name = "linux";
#elif defined(CORE_OS_ANDROID)
    name = "android";
#elif defined(CORE_OS_BSD)
    name = "bsd";
#elif defined(CORE_OS_APPLE)
    name = "macos";
#endif
    return vm.makeVar<VarStr>(loc, name);
}

FERAL_FUNC(getOSDistro, 0, false,
           "  fn() -> str\n"
           "Returns the current operating system's distribution, currently one of the following:\n"
           "- windows64\n"
           "- windows\n"
           "- macos\n"
           "- linux\n"
           "- android\n"
           "- freebsd\n"
           "- openbsd\n"
           "- bsdi\n"
           "- dragonflybsd\n"
           "- bsd (if none of the above BSD options work)")
{
    String distro;
#if defined(CORE_OS_WINDOWS)
#if defined(CORE_OS_WINDOWS64)
    distro = "windows64";
#else
    distro = "windows";
#endif
#elif defined(CORE_OS_LINUX)
    distro = "linux"; // arch,ubuntu,etc.
#elif defined(CORE_OS_ANDROID)
    distro = "android"; // version name - lollipop, marshmellow, etc.
#elif defined(CORE_OS_BSD)
#if defined(CORE_OS_FREEBSD)
    distro = "freebsd";
#elif defined(CORE_OS_NETBSD)
    distro = "netbsd";
#elif defined(CORE_OS_OPENBSD)
    distro = "openbsd";
#elif defined(CORE_OS_BSDI)
    distro = "bsdi";
#elif defined(CORE_OS_DRAGONFLYBSD)
    distro = "dragonflybsd";
#else
    distro = "bsd";
#endif
#elif defined(CORE_OS_APPLE)
    distro = "macos";
#endif
    return vm.makeVar<VarStr>(loc, distro);
}

FERAL_FUNC(exitNative, 1, false, "")
{
    EXPECT(VarInt, args[1], "exit code");
    vm.setExitCalled(true);
    vm.setExitCode(as<VarInt>(args[1])->getVal());
    return vm.getNil();
}

FERAL_FUNC(varExists, 1, true,
           "  fn(varName, varModule = nil) -> bool\n"
           "Returns `true` if the given variable `varName` exists.\n"
           "If `varModule` is an instance of Module, checks within that too.")
{
    EXPECT(VarStr, args[1], "variable name");
    StringRef varName = as<VarStr>(args[1])->getVal();
    VarModule *mod    = vm.getCurrModule();
    bool providedMod  = false;
    if(args.size() > 2 && args[2]->is<VarModule>()) {
        mod         = as<VarModule>(args[2]);
        providedMod = true;
        return mod->getAttr(varName) ? vm.getTrue() : vm.getFalse();
    }
    VarVars *moduleVars = vm.getVars();
    return moduleVars->getAttr(varName) || vm.getGlobal(varName) ? vm.getTrue() : vm.getFalse();
}

FERAL_FUNC(setMaxRecursionNative, 1, false, "")
{
    EXPECT(VarInt, args[1], "max recursion count");
    vm.setRecurseMax(as<VarInt>(args[1])->getVal());
    return vm.getNil();
}

FERAL_FUNC(getMaxRecursion, 0, false,
           "  fn() -> int\n"
           "Gets the maximum recursion limit for `VirtualMachine::execute()`.")
{
    return vm.makeVar<VarInt>(loc, vm.getRecurseMax());
}

FERAL_FUNC(
    addGlobalModulePaths, 1, true,
    "  fn(paths...) -> int\n"
    "Adds each of the provided paths to the global module paths' file.\n"
    "Feral searches each of the paths in this file for modules when the `import()` function is "
    "called.\n"
    "Returns the total number of paths added. If the path already exists in the module file, it "
    "doesn't count.")
{
    for(size_t i = 1; i < args.size(); ++i) {
        auto &arg = args[i];
        EXPECT(VarStr, arg, "path");
    }
    const char *modulePathsFile = vm.getGlobalModulePathsFile()->getVal().c_str();
    if(!fs::exists(modulePathsFile)) {
        FILE *f = fopen(modulePathsFile, "w");
        fclose(f);
    }
    String data;
    Vector<StringRef> existingData;
    if(File::readFile(modulePathsFile, data).getCode()) {
        existingData = utils::stringDelim(data, "\n");
    }
    FILE *f      = fopen(modulePathsFile, "a+");
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

FERAL_FUNC(
    removeGlobalModulePaths, 1, true,
    "  fn(paths...) -> int\n"
    "Removes each of the provided paths from the global module paths' file.\n"
    "Feral searches each of the paths in this file for modules when the `import()` function is "
    "called.\n"
    "Returns the total number of paths removed. If the path isn't present in the module file, it "
    "doesn't count.")
{
    for(size_t i = 1; i < args.size(); ++i) {
        auto &arg = args[i];
        EXPECT(VarStr, arg, "path");
    }
    const char *modulePathsFile = vm.getGlobalModulePathsFile()->getVal().c_str();
    if(!fs::exists(modulePathsFile)) {
        FILE *f = fopen(modulePathsFile, "w");
        fclose(f);
    }
    String data;
    Vector<StringRef> existingData;
    if(File::readFile(modulePathsFile, data).getCode()) {
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
    FILE *f = fopen(modulePathsFile, "w+");
    for(auto &data : existingData) {
        fwrite(data.data(), sizeof(char), data.size(), f);
        fwrite("\n", sizeof(char), 1, f);
    }
    fclose(f);
    return vm.makeVar<VarInt>(loc, removed);
}

INIT_DLL(Prelude)
{
    // global functions
    vm.addGlobal(loc, "ref", reference);
    vm.addGlobal(loc, "unref", unreference);
    vm.addGlobal(loc, "const", constant);
    vm.addGlobal(loc, "deconst", deconstant);
    vm.addGlobal(loc, "raise", raise);
    vm.addGlobal(loc, "evalCode", evalCode);
    vm.addGlobal(loc, "evalExpr", evalExpr);
    vm.addGlobal(loc, "enum", enumNew);
    vm.addGlobal(loc, "struct", structNew);
    vm.addGlobal(loc, "irange", intRange);

    // module functions

    vm.addLocal(loc, "crash", crash);
    vm.addLocal(loc, "addGlobalModulePaths", addGlobalModulePaths);
    vm.addLocal(loc, "removeGlobalModulePaths", removeGlobalModulePaths);
    vm.addLocal(loc, "exitNative", exitNative);
    vm.addLocal(loc, "setMaxRecursionNative", setMaxRecursionNative);
    vm.addLocal(loc, "getMaxRecursion", getMaxRecursion);
    vm.addLocal(loc, "getCurrModule", getCurrModule);
    vm.addLocal(loc, "getOSName", getOSName);
    vm.addLocal(loc, "getOSDistro", getOSDistro);
    // variadic as there can be no proxy for this function (to make args[2] (VarModule) optional)
    vm.addLocal(loc, "varExists", varExists);
    vm.addLocal(loc, "vecNew", vecNew);
    vm.addLocal(loc, "mapNew", mapNew);
    vm.addLocal(loc, "bytebufferNew", bytebufferNew);

    // VM altering variables
    vm.addLocal("moduleDirs", "", vm.getModuleDirs());
    vm.addLocal("moduleFinders", "", vm.getModuleFinders());
    vm.addLocal("args", "", vm.getCLIArgs());
    vm.addLocal("binaryPath", "", vm.getBinaryPath());
    vm.addLocal("installPath", "", vm.getInstallPath());
    vm.addLocal("tempPath", "", vm.getTempPath());
    vm.addLocal("libPath", "", vm.getLibPath());

    vm.makeLocal<VarInt>(loc, "versionMajor", "", PROJECT_MAJOR);
    vm.makeLocal<VarInt>(loc, "versionMinor", "", PROJECT_MINOR);
    vm.makeLocal<VarInt>(loc, "versionPatch", "", PROJECT_PATCH);
    vm.makeLocal<VarStr>(loc, "buildDate", "", BUILD_DATE);
    vm.makeLocal<VarStr>(loc, "buildCompiler", "", BUILD_COMPILER);
    vm.makeLocal<VarStr>(loc, "buildType", "", CMAKE_BUILD_TYPE);
    vm.makeLocal<VarStr>(loc, "minCmakeVersion", "", MIN_CMAKE_VERSION);
    vm.makeLocal<VarStr>(loc, "usedCmakeVersion", "", USED_CMAKE_VERSION);
    vm.makeLocal<VarInt>(loc, "DEFAULT_MAX_RECURSION", "", DEFAULT_MAX_RECURSE_COUNT);

    // fundamental functions for builtin types
    vm.addTypeFn<VarAll>(loc, "==", allEq);
    vm.addTypeFn<VarAll>(loc, "==", allEq);
    vm.addTypeFn<VarAll>(loc, "!=", allNe);
    vm.addTypeFn<VarAll>(loc, "\?\?", allNilCoalesce);
    vm.addTypeFn<VarAll>(loc, "_getDoc_", allGetDoc);
    vm.addTypeFn<VarAll>(loc, "_setDoc_", allSetDoc);
    vm.addTypeFn<VarAll>(loc, "_hasAttr_", allHasAttr);
    vm.addTypeFn<VarAll>(loc, "_getAttr_", allGetAttr);
    vm.addTypeFn<VarAll>(loc, "_getAttrs_", allGetAttrs);
    vm.addTypeFn<VarAll>(loc, "_setAttr_", allSetAttr);
    vm.addTypeFn<VarAll>(loc, "_type_", allGetType);
    vm.addTypeFn<VarAll>(loc, "_subType_", allGetSubType);
    vm.addTypeFn<VarAll>(loc, "_isType_", allIsType);
    vm.addTypeFn<VarAll>(loc, "_isSubType_", allIsSubType);
    vm.addTypeFn<VarAll>(loc, "_typeName_", allGetTypeName);

    // copy
    vm.addTypeFn<VarBool>(loc, "_copy_", boolCopy);
    vm.addTypeFn<VarInt>(loc, "_copy_", intCopy);
    vm.addTypeFn<VarFlt>(loc, "_copy_", fltCopy);
    vm.addTypeFn<VarStr>(loc, "_copy_", strCopy);
    vm.addTypeFn<VarVec>(loc, "_copy_", vecCopy);
    vm.addTypeFn<VarMap>(loc, "_copy_", mapCopy);
    vm.addTypeFn<VarBytebuffer>(loc, "_copy_", bytebufferCopy);

    // to bool
    vm.addTypeFn<VarAll>(loc, "bool", allToBool);
    vm.addTypeFn<VarNil>(loc, "bool", nilToBool);
    vm.addTypeFn<VarBool>(loc, "bool", boolToBool);
    vm.addTypeFn<VarInt>(loc, "bool", intToBool);
    vm.addTypeFn<VarFlt>(loc, "bool", fltToBool);
    vm.addTypeFn<VarStr>(loc, "bool", strToBool);
    vm.addTypeFn<VarVec>(loc, "bool", vecToBool);
    vm.addTypeFn<VarMap>(loc, "bool", mapToBool);
    vm.addTypeFn<VarTypeID>(loc, "bool", typeIDToBool);

    // to int
    vm.addTypeFn<VarNil>(loc, "int", nilToInt);
    vm.addTypeFn<VarBool>(loc, "int", boolToInt);
    vm.addTypeFn<VarInt>(loc, "int", intToInt);
    vm.addTypeFn<VarFlt>(loc, "int", fltToInt);
    vm.addTypeFn<VarStr>(loc, "intNative", strToIntNative);
    vm.addTypeFn<VarTypeID>(loc, "int", typeIDToInt);

    // to float
    vm.addTypeFn<VarNil>(loc, "flt", nilToFlt);
    vm.addTypeFn<VarBool>(loc, "flt", boolToFlt);
    vm.addTypeFn<VarInt>(loc, "flt", intToFlt);
    vm.addTypeFn<VarFlt>(loc, "flt", fltToFlt);
    vm.addTypeFn<VarStr>(loc, "flt", strToFlt);

    // to string
    vm.addTypeFn<VarAll>(loc, "str", allToStr);
    vm.addTypeFn<VarNil>(loc, "str", nilToStr);
    vm.addTypeFn<VarBool>(loc, "str", boolToStr);
    vm.addTypeFn<VarInt>(loc, "str", intToStr);
    vm.addTypeFn<VarFlt>(loc, "str", fltToStr);
    vm.addTypeFn<VarStr>(loc, "str", strToStr);
    vm.addTypeFn<VarPath>(loc, "str", pathToStr);
    vm.addTypeFn<VarTypeID>(loc, "str", typeIDToStr);
    vm.addTypeFn<VarFailure>(loc, "str", failureToStr);

    // to path
    vm.addTypeFn<VarStr>(loc, "path", strToPath);

    // core type functions

    // nil
    vm.addTypeFn<VarNil>(loc, "==", nilEQ);
    vm.addTypeFn<VarNil>(loc, "!=", nilNE);
    vm.addTypeFn<VarNil>(loc, "!", nilNot);

    // typeID
    vm.addTypeFn<VarTypeID>(loc, "==", typeIDEq);
    vm.addTypeFn<VarTypeID>(loc, "!=", typeIDNe);

    // bool
    vm.addTypeFn<VarBool>(loc, "==", boolEQ);
    vm.addTypeFn<VarBool>(loc, "!=", boolNE);

    vm.addTypeFn<VarBool>(loc, "!", boolNot);

    // int
    vm.addTypeFn<VarInt>(loc, "+", intAdd);
    vm.addTypeFn<VarInt>(loc, "-", intSub);
    vm.addTypeFn<VarInt>(loc, "*", intMul);
    vm.addTypeFn<VarInt>(loc, "/", intDiv);
    vm.addTypeFn<VarInt>(loc, "%", intMod);
    vm.addTypeFn<VarInt>(loc, "<<", intLShift);
    vm.addTypeFn<VarInt>(loc, ">>", intRShift);

    vm.addTypeFn<VarInt>(loc, "+=", intAssnAdd);
    vm.addTypeFn<VarInt>(loc, "-=", intAssnSub);
    vm.addTypeFn<VarInt>(loc, "*=", intAssnMul);
    vm.addTypeFn<VarInt>(loc, "/=", intAssnDiv);
    vm.addTypeFn<VarInt>(loc, "%=", intAssnMod);
    vm.addTypeFn<VarInt>(loc, "<<=", intAssnLShift);
    vm.addTypeFn<VarInt>(loc, ">>=", intAssnRShift);

    vm.addTypeFn<VarInt>(loc, "**", intPow);
    vm.addTypeFn<VarInt>(loc, "++x", intPreInc);
    vm.addTypeFn<VarInt>(loc, "x++", intPostInc);
    vm.addTypeFn<VarInt>(loc, "--x", intPreDec);
    vm.addTypeFn<VarInt>(loc, "x--", intPostDec);

    vm.addTypeFn<VarInt>(loc, "u-", intUSub);

    vm.addTypeFn<VarInt>(loc, "<", intLT);
    vm.addTypeFn<VarInt>(loc, ">", intGT);
    vm.addTypeFn<VarInt>(loc, "<=", intLE);
    vm.addTypeFn<VarInt>(loc, ">=", intGE);
    vm.addTypeFn<VarInt>(loc, "==", intEQ);
    vm.addTypeFn<VarInt>(loc, "!=", intNE);

    vm.addTypeFn<VarInt>(loc, "&", intBAnd);
    vm.addTypeFn<VarInt>(loc, "|", intBOr);
    vm.addTypeFn<VarInt>(loc, "^", intBXOr);
    vm.addTypeFn<VarInt>(loc, "~", intBNot);

    vm.addTypeFn<VarInt>(loc, "&=", intAssnBAnd);
    vm.addTypeFn<VarInt>(loc, "|=", intAssnBOr);
    vm.addTypeFn<VarInt>(loc, "^=", intAssnBXOr);

    vm.addTypeFn<VarInt>(loc, "sqrt", intSqRoot);
    vm.addTypeFn<VarInt>(loc, "popcnt", intPopCnt);

    // int iterator
    vm.addTypeFn<VarIntIterator>(loc, "next", getIntIteratorNext);

    // flt
    vm.addTypeFn<VarFlt>(loc, "+", fltAdd);
    vm.addTypeFn<VarFlt>(loc, "-", fltSub);
    vm.addTypeFn<VarFlt>(loc, "*", fltMul);
    vm.addTypeFn<VarFlt>(loc, "/", fltDiv);

    vm.addTypeFn<VarFlt>(loc, "+=", fltAssnAdd);
    vm.addTypeFn<VarFlt>(loc, "-=", fltAssnSub);
    vm.addTypeFn<VarFlt>(loc, "*=", fltAssnMul);
    vm.addTypeFn<VarFlt>(loc, "/=", fltAssnDiv);

    vm.addTypeFn<VarFlt>(loc, "++x", fltPreInc);
    vm.addTypeFn<VarFlt>(loc, "x++", fltPostInc);
    vm.addTypeFn<VarFlt>(loc, "--x", fltPreDec);
    vm.addTypeFn<VarFlt>(loc, "x--", fltPostDec);

    vm.addTypeFn<VarFlt>(loc, "u-", fltUSub);

    vm.addTypeFn<VarFlt>(loc, "**", fltPow);

    vm.addTypeFn<VarFlt>(loc, "<", fltLT);
    vm.addTypeFn<VarFlt>(loc, ">", fltGT);
    vm.addTypeFn<VarFlt>(loc, "<=", fltLE);
    vm.addTypeFn<VarFlt>(loc, ">=", fltGE);
    vm.addTypeFn<VarFlt>(loc, "==", fltEQ);
    vm.addTypeFn<VarFlt>(loc, "!=", fltNE);

    vm.addTypeFn<VarFlt>(loc, "round", fltRound);
    vm.addTypeFn<VarFlt>(loc, "sqrt", fltSqRoot);

    // string
    vm.addTypeFn<VarStr>(loc, "+", strAdd);
    vm.addTypeFn<VarStr>(loc, "*", strMul);
    vm.addTypeFn<VarStr>(loc, "/", strDiv);

    vm.addTypeFn<VarStr>(loc, "+=", strAssnAdd);
    vm.addTypeFn<VarStr>(loc, "*=", strAssnMul);

    vm.addTypeFn<VarStr>(loc, "<", strLT);
    vm.addTypeFn<VarStr>(loc, ">", strGT);
    vm.addTypeFn<VarStr>(loc, "<=", strLE);
    vm.addTypeFn<VarStr>(loc, ">=", strGE);
    vm.addTypeFn<VarStr>(loc, "==", strEQ);
    vm.addTypeFn<VarStr>(loc, "!=", strNE);

    vm.addTypeFn<VarStr>(loc, "at", strAt);
    vm.addTypeFn<VarStr>(loc, "[]", strAt);

    vm.addTypeFn<VarStr>(loc, "len", strSize);
    vm.addTypeFn<VarStr>(loc, "clear", strClear);
    vm.addTypeFn<VarStr>(loc, "empty", strEmpty);
    vm.addTypeFn<VarStr>(loc, "front", strFront);
    vm.addTypeFn<VarStr>(loc, "back", strBack);
    vm.addTypeFn<VarStr>(loc, "push", strPush);
    vm.addTypeFn<VarStr>(loc, "pop", strPop);
    vm.addTypeFn<VarStr>(loc, "isChAt", strIsChAt);
    vm.addTypeFn<VarStr>(loc, "set", strSetAt);
    vm.addTypeFn<VarStr>(loc, "insert", strInsert);
    vm.addTypeFn<VarStr>(loc, "erase", strErase);
    vm.addTypeFn<VarStr>(loc, "find", strFind);
    vm.addTypeFn<VarStr>(loc, "rfind", strRFind);
    vm.addTypeFn<VarStr>(loc, "substrNative", strSubstrNative);
    vm.addTypeFn<VarStr>(loc, "trim", strTrim);
    vm.addTypeFn<VarStr>(loc, "lower", strLower);
    vm.addTypeFn<VarStr>(loc, "upper", strUpper);
    vm.addTypeFn<VarStr>(loc, "replace", strReplace);
    vm.addTypeFn<VarStr>(loc, "splitNative", strSplitNative);
    vm.addTypeFn<VarStr>(loc, "startsWith", strStartsWith);
    vm.addTypeFn<VarStr>(loc, "endsWith", strEndsWith);
    vm.addTypeFn<VarStr>(loc, "fmt", strFormat);
    vm.addTypeFn<VarStr>(loc, "getBinStrFromHexStr", hexStrToBinStr);
    vm.addTypeFn<VarStr>(loc, "getUTF8CharFromBinStr", utf8CharFromBinStr);

    vm.addTypeFn<VarStr>(loc, "byt", byt);
    vm.addTypeFn<VarInt>(loc, "chr", chr);

    // vec

    vm.addTypeFn<VarVec>(loc, "len", vecSize);
    vm.addTypeFn<VarVec>(loc, "capacity", vecCapacity);
    vm.addTypeFn<VarVec>(loc, "isRef", vecIsRef);
    vm.addTypeFn<VarVec>(loc, "empty", vecEmpty);
    vm.addTypeFn<VarVec>(loc, "front", vecFront);
    vm.addTypeFn<VarVec>(loc, "back", vecBack);
    vm.addTypeFn<VarVec>(loc, "push", vecPush);
    vm.addTypeFn<VarVec>(loc, "pop", vecPop);
    vm.addTypeFn<VarVec>(loc, "clear", vecClear);
    vm.addTypeFn<VarVec>(loc, "erase", vecErase);
    vm.addTypeFn<VarVec>(loc, "insert", vecInsert);
    vm.addTypeFn<VarVec>(loc, "appendNative", vecAppendNative);
    vm.addTypeFn<VarVec>(loc, "swap", vecSwap);
    vm.addTypeFn<VarVec>(loc, "reverse", vecReverse);
    vm.addTypeFn<VarVec>(loc, "set", vecSetAt);
    vm.addTypeFn<VarVec>(loc, "at", vecAt);
    vm.addTypeFn<VarVec>(loc, "[]", vecAt);

    vm.addTypeFn<VarVec>(loc, "subNative", vecSubNative);
    vm.addTypeFn<VarVec>(loc, "sliceNative", vecSliceNative);

    vm.addTypeFn<VarVec>(loc, "each", vecEach);
    vm.addTypeFn<VarVecIterator>(loc, "next", vecIteratorNext);

    // map

    vm.addTypeFn<VarMap>(loc, "len", mapSize);
    vm.addTypeFn<VarMap>(loc, "isOrdered", mapIsOrdered);
    vm.addTypeFn<VarMap>(loc, "isRef", mapIsRef);
    vm.addTypeFn<VarMap>(loc, "empty", mapEmpty);
    vm.addTypeFn<VarMap>(loc, "insert", mapInsert);
    vm.addTypeFn<VarMap>(loc, "erase", mapErase);
    vm.addTypeFn<VarMap>(loc, "clear", mapClear);
    vm.addTypeFn<VarMap>(loc, "find", mapFind);
    vm.addTypeFn<VarMap>(loc, "at", mapAt);
    vm.addTypeFn<VarMap>(loc, "[]", mapAt);

    vm.addTypeFn<VarMap>(loc, "each", mapEach);
    vm.addTypeFn<VarMapIterator>(loc, "next", mapIteratorNext);

    // struct
    vm.addTypeFn<VarStructDef>(loc, "setTypeName", structDefSetTypeName);
    vm.addTypeFn<VarStructDef>(loc, "len", structDefLen);

    vm.addTypeFn<VarStruct>(loc, "str", structToStr);
    vm.addTypeFn<VarStruct>(loc, "len", structLen);

    // bytebuffer

    vm.addTypeFn<VarBytebuffer>(loc, "len", bytebufferLen);
    vm.addTypeFn<VarBytebuffer>(loc, "capacity", bytebufferCapacity);
    vm.addTypeFn<VarBytebuffer>(loc, "str", bytebufferToStr);

    // path

    vm.addTypeFn<VarPath>(loc, "_copy_", pathCopy);
    vm.addTypeFn<VarPath>(loc, "==", pathEQ);
    vm.addTypeFn<VarPath>(loc, "!=", pathNE);
    vm.addTypeFn<VarPath>(loc, ">", pathGT);
    vm.addTypeFn<VarPath>(loc, ">=", pathGE);
    vm.addTypeFn<VarPath>(loc, "<", pathLT);
    vm.addTypeFn<VarPath>(loc, "<=", pathLE);
    vm.addTypeFn<VarPath>(loc, "/", pathJoin);
    vm.addTypeFn<VarPath>(loc, "/=", pathAssnJoin);
    vm.addTypeFn<VarPath>(loc, "clear", pathClear);
    vm.addTypeFn<VarPath>(loc, "len", pathLen);
    vm.addTypeFn<VarPath>(loc, "empty", pathEmpty);
    vm.addTypeFn<VarPath>(loc, "isAbsolute", pathIsAbsolute);
    vm.addTypeFn<VarPath>(loc, "isRelative", pathIsRelative);
    vm.addTypeFn<VarPath>(loc, "hasRoot", pathHasRoot);
    vm.addTypeFn<VarPath>(loc, "hasRootName", pathHasRootName);
    vm.addTypeFn<VarPath>(loc, "hasRootDir", pathHasRootDir);
    vm.addTypeFn<VarPath>(loc, "hasRelative", pathHasRelative);
    vm.addTypeFn<VarPath>(loc, "hasParent", pathHasParent);
    vm.addTypeFn<VarPath>(loc, "hasFile", pathHasFile);
    vm.addTypeFn<VarPath>(loc, "hasFileName", pathHasFileName);
    vm.addTypeFn<VarPath>(loc, "hasFileExt", pathHasFileExt);
    vm.addTypeFn<VarPath>(loc, "root", pathRoot);
    vm.addTypeFn<VarPath>(loc, "rootName", pathRootName);
    vm.addTypeFn<VarPath>(loc, "rootDir", pathRootDir);
    vm.addTypeFn<VarPath>(loc, "normal", pathNormal);
    vm.addTypeFn<VarPath>(loc, "absolute", pathAbsolute);
    vm.addTypeFn<VarPath>(loc, "relative", pathRelative);
    vm.addTypeFn<VarPath>(loc, "relativeTo", pathRelativeTo);
    vm.addTypeFn<VarPath>(loc, "parent", pathParent);
    vm.addTypeFn<VarPath>(loc, "file", pathFile);
    vm.addTypeFn<VarPath>(loc, "fileName", pathFileName);
    vm.addTypeFn<VarPath>(loc, "fileExt", pathFileExt);

    // file

    vm.addTypeFn<VarFile>(loc, "lines", fileLines);
    vm.addTypeFn<VarFile>(loc, "seek", fileSeek);
    vm.addTypeFn<VarFile>(loc, "eachLine", fileEachLine);
    vm.addTypeFn<VarFile>(loc, "readAll", fileReadAll);
    vm.addTypeFn<VarFile>(loc, "readBlocks", fileReadBlocks);

    vm.addTypeFn<VarFileIterator>(loc, "next", fileIteratorNext);

    // module

    vm.addTypeFn<VarModule>(loc, "_path_", moduleGetPath);

    return true;
}

} // namespace fer