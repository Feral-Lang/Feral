#include "FS.hpp" // used by File.hpp.in
#include "VM/Interpreter.hpp"

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
    if(args[1]->is<VarNil>()) args[0]->setDoc(mem, nullptr);
    else args[0]->setDoc(mem, as<VarStr>(vm.copyVar(loc, args[1])));
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
    in->getAttrList(vm.getMemoryManager(), res);
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
    in->setAttr(vm.getMemoryManager(), attr, args[2], true);
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

FERAL_FUNC(allCopy, 0, false,
           "  var.fn() -> var\n"
           "Returns a copy of `var`.")
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
FERAL_FUNC(reference, 1, false,
           "  fn(var) -> var\n"
           "Returns the argument itself, but with loadAsRef internal "
           "variable set as true.\n"
           "This ensures that the next time a new object is created "
           "using `var` as the value, `var` is not copied.")
{
    // any const marked variable cannot be marked with loadAsRef()
    if(!args[1]->isConst()) args[1]->setLoadAsRef();
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
    Vars &moduleVars = vm.getVars();
    return moduleVars.get(varName) || vm.getGlobal(varName) ? vm.getTrue() : vm.getFalse();
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
    if(fs::read(modulePathsFile, data).getCode()) { existingData = utils::stringDelim(data, "\n"); }
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
    if(fs::read(modulePathsFile, data).getCode()) { existingData = utils::stringDelim(data, "\n"); }
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

INIT_MODULE(Prelude)
{
    VarModule *mod = vm.getCurrModule(); // prelude module

    // global functions
    vm.addNativeFn(loc, "ref", reference);
    vm.addNativeFn(loc, "unref", unreference);
    vm.addNativeFn(loc, "const", constant);
    vm.addNativeFn(loc, "deconst", deconstant);
    vm.addNativeFn(loc, "raise", raise);
    vm.addNativeFn(loc, "evalCode", evalCode);
    vm.addNativeFn(loc, "evalExpr", evalExpr);
    vm.addNativeFn(loc, "enum", createEnum);
    vm.addNativeFn(loc, "struct", createStruct);
    vm.addNativeFn(loc, "irange", intRange);

    // module functions
    mod->addNativeFn(vm, "addGlobalModulePaths", addGlobalModulePaths);
    mod->addNativeFn(vm, "removeGlobalModulePaths", removeGlobalModulePaths);
    mod->addNativeFn(vm, "exitNative", exitNative);
    mod->addNativeFn(vm, "setMaxRecursionNative", setMaxRecursionNative);
    mod->addNativeFn(vm, "getMaxRecursion", getMaxRecursion);
    mod->addNativeFn(vm, "getCurrModule", getCurrModule);
    mod->addNativeFn(vm, "getOSName", getOSName);
    mod->addNativeFn(vm, "getOSDistro", getOSDistro);
    // variadic as there can be no proxy for this function (to make args[2] (VarModule) optional)
    mod->addNativeFn(vm, "varExists", varExists);
    mod->addNativeFn(vm, "vecNew", vecNew);
    mod->addNativeFn(vm, "mapNew", mapNew);
    mod->addNativeFn(vm, "bytebufferNew", bytebufferNew);
    // file/filesystem
    mod->addNativeFn(vm, "fsFopen", fsFopen);
    mod->addNativeFn(vm, "fsWalkDir", fsWalkDir);
    mod->addNativeFn(vm, "fsAbsPath", fsAbsPath);
    // file descriptor
    mod->addNativeFn(vm, "fsFdOpen", fdOpen);
    mod->addNativeFn(vm, "fsFdRead", fdRead);
    mod->addNativeFn(vm, "fsFdWrite", fdWrite);
    mod->addNativeFn(vm, "fsFdClose", fdClose);
    // files and dirs
    mod->addNativeFn(vm, "fsExists", fsExists);
    mod->addNativeFn(vm, "fsInstall", fsInstall);
    mod->addNativeFn(vm, "fsMklink", fsMklink);
    mod->addNativeFn(vm, "fsMove", fsMove);
    mod->addNativeFn(vm, "fsMkdir", fsMkdir);
    mod->addNativeFn(vm, "fsRemove", fsRemove);
    mod->addNativeFn(vm, "fsCopy", fsCopy);

    // VM altering variables
    mod->addNativeVar(vm, "moduleDirs", "", vm.getModuleDirs());
    mod->addNativeVar(vm, "moduleFinders", "", vm.getModuleFinders());
    mod->addNativeVar(vm, "args", "", vm.getCLIArgs());
    mod->addNativeVar(vm, "binaryPath", "", vm.getBinaryPath());
    mod->addNativeVar(vm, "installPath", "", vm.getInstallPath());
    mod->addNativeVar(vm, "tempPath", "", vm.getTempPath());
    mod->addNativeVar(vm, "libPath", "", vm.getLibPath());
    mod->addNativeVar(vm, "versionMajor", "", vm.makeVar<VarInt>(loc, PROJECT_MAJOR));
    mod->addNativeVar(vm, "versionMinor", "", vm.makeVar<VarInt>(loc, PROJECT_MINOR));
    mod->addNativeVar(vm, "versionPatch", "", vm.makeVar<VarInt>(loc, PROJECT_PATCH));
    mod->addNativeVar(vm, "buildDate", "", vm.makeVar<VarStr>(loc, BUILD_DATE));
    mod->addNativeVar(vm, "buildCompiler", "", vm.makeVar<VarStr>(loc, BUILD_COMPILER));
    mod->addNativeVar(vm, "buildType", "", vm.makeVar<VarStr>(loc, CMAKE_BUILD_TYPE));
    mod->addNativeVar(vm, "minCmakeVersion", "", vm.makeVar<VarStr>(loc, MIN_CMAKE_VERSION));
    mod->addNativeVar(vm, "usedCmakeVersion", "", vm.makeVar<VarStr>(loc, USED_CMAKE_VERSION));
    mod->addNativeVar(vm, "DEFAULT_MAX_RECURSION", "",
                      vm.makeVar<VarInt>(loc, DEFAULT_MAX_RECURSE_COUNT));

    // fundamental functions for builtin types
    vm.addNativeTypeFn<VarAll>(loc, "==", allEq);
    vm.addNativeTypeFn<VarAll>(loc, "!=", allNe);
    vm.addNativeTypeFn<VarAll>(loc, "\?\?", allNilCoalesce);
    vm.addNativeTypeFn<VarAll>(loc, "copy", allCopy);
    vm.addNativeTypeFn<VarAll>(loc, "_getDoc_", allGetDoc);
    vm.addNativeTypeFn<VarAll>(loc, "_setDoc_", allSetDoc);
    vm.addNativeTypeFn<VarAll>(loc, "_hasAttr_", allHasAttr);
    vm.addNativeTypeFn<VarAll>(loc, "_getAttr_", allGetAttr);
    vm.addNativeTypeFn<VarAll>(loc, "_getAttrs_", allGetAttrs);
    vm.addNativeTypeFn<VarAll>(loc, "_setAttr_", allSetAttr);
    vm.addNativeTypeFn<VarAll>(loc, "_type_", allGetType);
    vm.addNativeTypeFn<VarAll>(loc, "_subType_", allGetSubType);
    vm.addNativeTypeFn<VarAll>(loc, "_isType_", allIsType);
    vm.addNativeTypeFn<VarAll>(loc, "_isSubType_", allIsSubType);
    vm.addNativeTypeFn<VarAll>(loc, "_typeName_", allGetTypeName);

    // to bool
    vm.addNativeTypeFn<VarAll>(loc, "bool", allToBool);
    vm.addNativeTypeFn<VarNil>(loc, "bool", nilToBool);
    vm.addNativeTypeFn<VarBool>(loc, "bool", boolToBool);
    vm.addNativeTypeFn<VarInt>(loc, "bool", intToBool);
    vm.addNativeTypeFn<VarFlt>(loc, "bool", fltToBool);
    vm.addNativeTypeFn<VarStr>(loc, "bool", strToBool);
    vm.addNativeTypeFn<VarVec>(loc, "bool", vecToBool);
    vm.addNativeTypeFn<VarMap>(loc, "bool", mapToBool);
    vm.addNativeTypeFn<VarTypeID>(loc, "bool", typeIDToBool);

    // to int
    vm.addNativeTypeFn<VarNil>(loc, "int", nilToInt);
    vm.addNativeTypeFn<VarBool>(loc, "int", boolToInt);
    vm.addNativeTypeFn<VarInt>(loc, "int", intToInt);
    vm.addNativeTypeFn<VarFlt>(loc, "int", fltToInt);
    vm.addNativeTypeFn<VarStr>(loc, "intNative", strToIntNative);
    vm.addNativeTypeFn<VarTypeID>(loc, "int", typeIDToInt);

    // to float
    vm.addNativeTypeFn<VarNil>(loc, "flt", nilToFlt);
    vm.addNativeTypeFn<VarBool>(loc, "flt", boolToFlt);
    vm.addNativeTypeFn<VarInt>(loc, "flt", intToFlt);
    vm.addNativeTypeFn<VarFlt>(loc, "flt", fltToFlt);
    vm.addNativeTypeFn<VarStr>(loc, "flt", strToFlt);

    // to string
    vm.addNativeTypeFn<VarAll>(loc, "str", allToStr);
    vm.addNativeTypeFn<VarNil>(loc, "str", nilToStr);
    vm.addNativeTypeFn<VarBool>(loc, "str", boolToStr);
    vm.addNativeTypeFn<VarInt>(loc, "str", intToStr);
    vm.addNativeTypeFn<VarFlt>(loc, "str", fltToStr);
    vm.addNativeTypeFn<VarStr>(loc, "str", strToStr);
    vm.addNativeTypeFn<VarTypeID>(loc, "str", typeIDToStr);
    vm.addNativeTypeFn<VarFailure>(loc, "str", failureToStr);

    // core type functions

    // nil
    vm.addNativeTypeFn<VarNil>(loc, "==", nilEQ);
    vm.addNativeTypeFn<VarNil>(loc, "!=", nilNE);
    vm.addNativeTypeFn<VarNil>(loc, "!", nilNot);

    // typeID
    vm.addNativeTypeFn<VarTypeID>(loc, "==", typeIDEq);
    vm.addNativeTypeFn<VarTypeID>(loc, "!=", typeIDNe);

    // bool
    vm.addNativeTypeFn<VarBool>(loc, "==", boolEQ);
    vm.addNativeTypeFn<VarBool>(loc, "!=", boolNE);

    vm.addNativeTypeFn<VarBool>(loc, "!", boolNot);

    // int
    vm.addNativeTypeFn<VarInt>(loc, "+", intAdd);
    vm.addNativeTypeFn<VarInt>(loc, "-", intSub);
    vm.addNativeTypeFn<VarInt>(loc, "*", intMul);
    vm.addNativeTypeFn<VarInt>(loc, "/", intDiv);
    vm.addNativeTypeFn<VarInt>(loc, "%", intMod);
    vm.addNativeTypeFn<VarInt>(loc, "<<", intLShift);
    vm.addNativeTypeFn<VarInt>(loc, ">>", intRShift);

    vm.addNativeTypeFn<VarInt>(loc, "+=", intAssnAdd);
    vm.addNativeTypeFn<VarInt>(loc, "-=", intAssnSub);
    vm.addNativeTypeFn<VarInt>(loc, "*=", intAssnMul);
    vm.addNativeTypeFn<VarInt>(loc, "/=", intAssnDiv);
    vm.addNativeTypeFn<VarInt>(loc, "%=", intAssnMod);
    vm.addNativeTypeFn<VarInt>(loc, "<<=", intAssnLShift);
    vm.addNativeTypeFn<VarInt>(loc, ">>=", intAssnRShift);

    vm.addNativeTypeFn<VarInt>(loc, "**", intPow);
    vm.addNativeTypeFn<VarInt>(loc, "++x", intPreInc);
    vm.addNativeTypeFn<VarInt>(loc, "x++", intPostInc);
    vm.addNativeTypeFn<VarInt>(loc, "--x", intPreDec);
    vm.addNativeTypeFn<VarInt>(loc, "x--", intPostDec);

    vm.addNativeTypeFn<VarInt>(loc, "u-", intUSub);

    vm.addNativeTypeFn<VarInt>(loc, "<", intLT);
    vm.addNativeTypeFn<VarInt>(loc, ">", intGT);
    vm.addNativeTypeFn<VarInt>(loc, "<=", intLE);
    vm.addNativeTypeFn<VarInt>(loc, ">=", intGE);
    vm.addNativeTypeFn<VarInt>(loc, "==", intEQ);
    vm.addNativeTypeFn<VarInt>(loc, "!=", intNE);

    vm.addNativeTypeFn<VarInt>(loc, "&", intBAnd);
    vm.addNativeTypeFn<VarInt>(loc, "|", intBOr);
    vm.addNativeTypeFn<VarInt>(loc, "^", intBXOr);
    vm.addNativeTypeFn<VarInt>(loc, "~", intBNot);

    vm.addNativeTypeFn<VarInt>(loc, "&=", intAssnBAnd);
    vm.addNativeTypeFn<VarInt>(loc, "|=", intAssnBOr);
    vm.addNativeTypeFn<VarInt>(loc, "^=", intAssnBXOr);

    vm.addNativeTypeFn<VarInt>(loc, "sqrt", intSqRoot);
    vm.addNativeTypeFn<VarInt>(loc, "popcnt", intPopCnt);

    // int iterator
    vm.addNativeTypeFn<VarIntIterator>(loc, "next", getIntIteratorNext);

    // flt
    vm.addNativeTypeFn<VarFlt>(loc, "+", fltAdd);
    vm.addNativeTypeFn<VarFlt>(loc, "-", fltSub);
    vm.addNativeTypeFn<VarFlt>(loc, "*", fltMul);
    vm.addNativeTypeFn<VarFlt>(loc, "/", fltDiv);

    vm.addNativeTypeFn<VarFlt>(loc, "+=", fltAssnAdd);
    vm.addNativeTypeFn<VarFlt>(loc, "-=", fltAssnSub);
    vm.addNativeTypeFn<VarFlt>(loc, "*=", fltAssnMul);
    vm.addNativeTypeFn<VarFlt>(loc, "/=", fltAssnDiv);

    vm.addNativeTypeFn<VarFlt>(loc, "++x", fltPreInc);
    vm.addNativeTypeFn<VarFlt>(loc, "x++", fltPostInc);
    vm.addNativeTypeFn<VarFlt>(loc, "--x", fltPreDec);
    vm.addNativeTypeFn<VarFlt>(loc, "x--", fltPostDec);

    vm.addNativeTypeFn<VarFlt>(loc, "u-", fltUSub);

    vm.addNativeTypeFn<VarFlt>(loc, "**", fltPow);

    vm.addNativeTypeFn<VarFlt>(loc, "<", fltLT);
    vm.addNativeTypeFn<VarFlt>(loc, ">", fltGT);
    vm.addNativeTypeFn<VarFlt>(loc, "<=", fltLE);
    vm.addNativeTypeFn<VarFlt>(loc, ">=", fltGE);
    vm.addNativeTypeFn<VarFlt>(loc, "==", fltEQ);
    vm.addNativeTypeFn<VarFlt>(loc, "!=", fltNE);

    vm.addNativeTypeFn<VarFlt>(loc, "round", fltRound);
    vm.addNativeTypeFn<VarFlt>(loc, "sqrt", fltSqRoot);

    // string
    vm.addNativeTypeFn<VarStr>(loc, "+", strAdd);
    vm.addNativeTypeFn<VarStr>(loc, "*", strMul);

    vm.addNativeTypeFn<VarStr>(loc, "+=", strAssnAdd);
    vm.addNativeTypeFn<VarStr>(loc, "*=", strAssnMul);

    vm.addNativeTypeFn<VarStr>(loc, "<", strLT);
    vm.addNativeTypeFn<VarStr>(loc, ">", strGT);
    vm.addNativeTypeFn<VarStr>(loc, "<=", strLE);
    vm.addNativeTypeFn<VarStr>(loc, ">=", strGE);
    vm.addNativeTypeFn<VarStr>(loc, "==", strEQ);
    vm.addNativeTypeFn<VarStr>(loc, "!=", strNE);

    vm.addNativeTypeFn<VarStr>(loc, "at", strAt);
    vm.addNativeTypeFn<VarStr>(loc, "[]", strAt);

    vm.addNativeTypeFn<VarStr>(loc, "len", strSize);
    vm.addNativeTypeFn<VarStr>(loc, "clear", strClear);
    vm.addNativeTypeFn<VarStr>(loc, "empty", strEmpty);
    vm.addNativeTypeFn<VarStr>(loc, "front", strFront);
    vm.addNativeTypeFn<VarStr>(loc, "back", strBack);
    vm.addNativeTypeFn<VarStr>(loc, "push", strPush);
    vm.addNativeTypeFn<VarStr>(loc, "pop", strPop);
    vm.addNativeTypeFn<VarStr>(loc, "isChAt", strIsChAt);
    vm.addNativeTypeFn<VarStr>(loc, "set", strSetAt);
    vm.addNativeTypeFn<VarStr>(loc, "insert", strInsert);
    vm.addNativeTypeFn<VarStr>(loc, "erase", strErase);
    vm.addNativeTypeFn<VarStr>(loc, "find", strFind);
    vm.addNativeTypeFn<VarStr>(loc, "rfind", strRFind);
    vm.addNativeTypeFn<VarStr>(loc, "substrNative", strSubstrNative);
    vm.addNativeTypeFn<VarStr>(loc, "trim", strTrim);
    vm.addNativeTypeFn<VarStr>(loc, "lower", strLower);
    vm.addNativeTypeFn<VarStr>(loc, "upper", strUpper);
    vm.addNativeTypeFn<VarStr>(loc, "replace", strReplace);
    vm.addNativeTypeFn<VarStr>(loc, "splitNative", strSplitNative);
    vm.addNativeTypeFn<VarStr>(loc, "startsWith", strStartsWith);
    vm.addNativeTypeFn<VarStr>(loc, "endsWith", strEndsWith);
    vm.addNativeTypeFn<VarStr>(loc, "fmt", strFormat);
    vm.addNativeTypeFn<VarStr>(loc, "getBinStrFromHexStr", hexStrToBinStr);
    vm.addNativeTypeFn<VarStr>(loc, "getUTF8CharFromBinStr", utf8CharFromBinStr);

    vm.addNativeTypeFn<VarStr>(loc, "byt", byt);
    vm.addNativeTypeFn<VarInt>(loc, "chr", chr);

    // vec

    vm.addNativeTypeFn<VarVec>(loc, "len", vecSize);
    vm.addNativeTypeFn<VarVec>(loc, "capacity", vecCapacity);
    vm.addNativeTypeFn<VarVec>(loc, "isRef", vecIsRef);
    vm.addNativeTypeFn<VarVec>(loc, "empty", vecEmpty);
    vm.addNativeTypeFn<VarVec>(loc, "front", vecFront);
    vm.addNativeTypeFn<VarVec>(loc, "back", vecBack);
    vm.addNativeTypeFn<VarVec>(loc, "push", vecPush);
    vm.addNativeTypeFn<VarVec>(loc, "pop", vecPop);
    vm.addNativeTypeFn<VarVec>(loc, "clear", vecClear);
    vm.addNativeTypeFn<VarVec>(loc, "erase", vecErase);
    vm.addNativeTypeFn<VarVec>(loc, "insert", vecInsert);
    vm.addNativeTypeFn<VarVec>(loc, "appendNative", vecAppendNative);
    vm.addNativeTypeFn<VarVec>(loc, "swap", vecSwap);
    vm.addNativeTypeFn<VarVec>(loc, "reverse", vecReverse);
    vm.addNativeTypeFn<VarVec>(loc, "set", vecSetAt);
    vm.addNativeTypeFn<VarVec>(loc, "at", vecAt);
    vm.addNativeTypeFn<VarVec>(loc, "[]", vecAt);

    vm.addNativeTypeFn<VarVec>(loc, "subNative", vecSubNative);
    vm.addNativeTypeFn<VarVec>(loc, "sliceNative", vecSliceNative);

    vm.addNativeTypeFn<VarVec>(loc, "each", vecEach);
    vm.addNativeTypeFn<VarVecIterator>(loc, "next", vecIteratorNext);

    // map

    vm.addNativeTypeFn<VarMap>(loc, "len", mapSize);
    vm.addNativeTypeFn<VarMap>(loc, "isRef", mapIsRef);
    vm.addNativeTypeFn<VarMap>(loc, "empty", mapEmpty);
    vm.addNativeTypeFn<VarMap>(loc, "insert", mapInsert);
    vm.addNativeTypeFn<VarMap>(loc, "erase", mapErase);
    vm.addNativeTypeFn<VarMap>(loc, "clear", mapClear);
    vm.addNativeTypeFn<VarMap>(loc, "find", mapFind);
    vm.addNativeTypeFn<VarMap>(loc, "at", mapAt);
    vm.addNativeTypeFn<VarMap>(loc, "[]", mapAt);

    vm.addNativeTypeFn<VarMap>(loc, "each", mapEach);
    vm.addNativeTypeFn<VarMapIterator>(loc, "next", mapIteratorNext);

    // struct
    vm.addNativeTypeFn<VarStructDef>(loc, "setTypeName", structDefSetTypeName);
    vm.addNativeTypeFn<VarStructDef>(loc, "len", structDefLen);

    vm.addNativeTypeFn<VarStruct>(loc, "str", structToStr);
    vm.addNativeTypeFn<VarStruct>(loc, "len", structLen);

    // bytebuffer

    vm.addNativeTypeFn<VarBytebuffer>(loc, "len", bytebufferLen);
    vm.addNativeTypeFn<VarBytebuffer>(loc, "capacity", bytebufferCapacity);
    vm.addNativeTypeFn<VarBytebuffer>(loc, "str", bytebufferToStr);

    // file

    vm.addNativeTypeFn<VarFile>(loc, "reopenNative", fileReopenNative);
    vm.addNativeTypeFn<VarFile>(loc, "lines", fileLines);
    vm.addNativeTypeFn<VarFile>(loc, "seek", fileSeek);
    vm.addNativeTypeFn<VarFile>(loc, "eachLine", fileEachLine);
    vm.addNativeTypeFn<VarFile>(loc, "readAll", fileReadAll);
    vm.addNativeTypeFn<VarFile>(loc, "readBlocks", fileReadBlocks);

    vm.addNativeTypeFn<VarFileIterator>(loc, "next", fileIteratorNext);

    // module

    vm.addNativeTypeFn<VarModule>(loc, "_path_", moduleGetPath);

    // constants (for file/filesystem)
    // stdin, stdout, stderr file descriptors
    mod->addNativeVar(vm, "fsStdin", "", vm.makeVar<VarInt>(loc, STDIN_FILENO));
    mod->addNativeVar(vm, "fsStdout", "", vm.makeVar<VarInt>(loc, STDOUT_FILENO));
    mod->addNativeVar(vm, "fsStderr", "", vm.makeVar<VarInt>(loc, STDERR_FILENO));
    // fs.walkdir()
    mod->addNativeVar(vm, "FS_WALK_FILES", "", vm.makeVar<VarInt>(loc, WalkEntry::FILES));
    mod->addNativeVar(vm, "FS_WALK_DIRS", "", vm.makeVar<VarInt>(loc, WalkEntry::DIRS));
    mod->addNativeVar(vm, "FS_WALK_RECURSE", "", vm.makeVar<VarInt>(loc, WalkEntry::RECURSE));
    // <file>.seek()
    mod->addNativeVar(vm, "FS_SEEK_SET", "", vm.makeVar<VarInt>(loc, SEEK_SET));
    mod->addNativeVar(vm, "FS_SEEK_CUR", "", vm.makeVar<VarInt>(loc, SEEK_CUR));
    mod->addNativeVar(vm, "FS_SEEK_END", "", vm.makeVar<VarInt>(loc, SEEK_END));
    // file descriptor flags
    mod->addNativeVar(vm, "FS_O_RDONLY", "", vm.makeVar<VarInt>(loc, O_RDONLY));
    mod->addNativeVar(vm, "FS_O_WRONLY", "", vm.makeVar<VarInt>(loc, O_WRONLY));
    mod->addNativeVar(vm, "FS_O_RDWR", "", vm.makeVar<VarInt>(loc, O_RDWR));
    mod->addNativeVar(vm, "FS_O_APPEND", "", vm.makeVar<VarInt>(loc, O_APPEND));
    mod->addNativeVar(vm, "FS_O_CREAT", "", vm.makeVar<VarInt>(loc, O_CREAT));
#if defined(CORE_OS_LINUX) || defined(CORE_OS_APPLE)
    mod->addNativeVar(vm, "FS_O_DSYNC", "", vm.makeVar<VarInt>(loc, O_DSYNC));
#endif
    mod->addNativeVar(vm, "FS_O_EXCL", "", vm.makeVar<VarInt>(loc, O_EXCL));
#if !defined(CORE_OS_WINDOWS)
    mod->addNativeVar(vm, "FS_O_NOCTTY", "", vm.makeVar<VarInt>(loc, O_NOCTTY));
    mod->addNativeVar(vm, "FS_O_NONBLOCK", "", vm.makeVar<VarInt>(loc, O_NONBLOCK));
    mod->addNativeVar(vm, "FS_O_SYNC", "", vm.makeVar<VarInt>(loc, O_SYNC));
#endif
#if defined(CORE_OS_LINUX)
    mod->addNativeVar(vm, "FS_O_RSYNC", "", vm.makeVar<VarInt>(loc, O_RSYNC));
#endif
    mod->addNativeVar(vm, "FS_O_TRUNC", "", vm.makeVar<VarInt>(loc, O_TRUNC));

    return true;
}

} // namespace fer