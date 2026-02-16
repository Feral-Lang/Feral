#include "VM/GlobalState.hpp"

#include "Env.hpp"
#include "Error.hpp"
#include "FS.hpp"
#include "Utils.hpp"
#include "VM/CoreFuncs.hpp"

#if defined(CORE_OS_WINDOWS)
#include <chrono>    // because MSVC complains about missing header while Linux doesn't :shrug:
#include <Windows.h> // for libloaderapi.h, which contains AddDllDirectory() and RemoveDllDirectory()
#endif

namespace fer
{

Var *loadModule(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                const StringMap<AssnArgData> &assnArgs);

#if defined(CORE_OS_WINDOWS)
static StringMap<DLL_DIRECTORY_COOKIE> dllDirectories;
bool addDLLDirectory(StringRef dir);
void remDLLDirectories();
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// GlobalState //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

GlobalState::GlobalState(args::ArgParser &argparser, ParseSourceFn parseSourceFn)
    : mem("VM::Main"), managedAllocator(mem, "VM::ManagedAllocator"), argparser(argparser),
      parseSourceFn(parseSourceFn), vmCount(0), recurseMax(DEFAULT_MAX_RECURSE_COUNT)
{}
GlobalState::~GlobalState() {}

bool GlobalState::init(VirtualMachine &vm)
{
#if defined(CORE_OS_WINDOWS)
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                             LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32 |
                             LOAD_LIBRARY_SEARCH_USER_DIRS);
#endif

    prelude = "prelude/prelude";

    basicErrHandler = vm.incVarRef(vm.genNativeFn({}, "basicErrorHandler", basicErrorHandler));
    globals         = vm.incVarRef(vm.makeVar<VarMap>({}, 0, false));
    moduleDirs      = vm.incVarRef(vm.makeVar<VarVec>({}, 2, false));
    moduleFinders   = vm.incVarRef(vm.makeVar<VarVec>({}, 2, false));
    binaryPath      = vm.incVarRef(vm.makeVar<VarStr>({}, env::getProcPath()));
    installPath =
        vm.incVarRef(vm.makeVar<VarStr>({}, fs::parentDir(fs::parentDir(binaryPath->getVal()))));
    // To make sure if user installs Feral in, say, `/usr`,
    // Feral doesn't attempt using `/usr/tmp` as the temp path.
    if(installPath->getVal().ends_with(".feral")) {
        tempPath = vm.incVarRef(vm.makeVar<VarStr>({}, installPath->getVal() + "/tmp"));
        libPath  = vm.incVarRef(vm.makeVar<VarStr>({}, installPath->getVal() + "/lib/feral"));
        globalModulesPath =
            vm.incVarRef(vm.makeVar<VarStr>({}, libPath->getVal() + "/.modulePaths"));
    } else {
        tempPath = vm.incVarRef(vm.makeVar<VarStr>({}, "/tmp/feral." + env::get("USERNAME")));
        libPath  = vm.incVarRef(vm.makeVar<VarStr>({}, installPath->getVal() + "/lib/feral"));
        globalModulesPath =
            vm.incVarRef(vm.makeVar<VarStr>({}, libPath->getVal() + "/.modulePaths"));
    }
    tru  = vm.incVarRef(vm.makeVar<VarBool>({}, true));
    fals = vm.incVarRef(vm.makeVar<VarBool>({}, false));
    nil  = vm.incVarRef(vm.makeVar<VarNil>({}));

    binaryPath->setConst();
    installPath->setConst();
    tempPath->setConst();
    libPath->setConst();
    globalModulesPath->setConst();
    basicErrHandler->setConst();
    tru->setConst();
    fals->setConst();
    nil->setConst();

    Span<StringRef> vmArgs = argparser.getPassthrough();

    cmdargs = vm.incVarRef(vm.makeVar<VarVec>({}, vmArgs.size(), false));
    for(size_t i = 0; i < vmArgs.size(); ++i) {
        auto &a = vmArgs[i];
        cmdargs->push(vm, vm.makeVar<VarStr>({}, a), true);
    }
    cmdargs->setConst();

    moduleDirs->insert(vm, 0, libPath, true);

    // FERAL_PATHS supercedes the install path, ie. I can even run a custom stdlib if I want :D
    String feralPaths = env::get("FERAL_PATHS");
    for(auto &_path : utils::stringDelim(feralPaths, ";")) {
        VarStr *moduleLoc = vm.makeVar<VarStr>({}, _path);
        moduleDirs->insert(vm, 0, moduleLoc, true);
    }

    // Global .modulePaths file.
    // The path of a package is added to it when it's installed from command line via package
    // manager.
    vm.tryAddModulePathsFromFile(globalModulesPath->getVal().c_str());

#if defined(CORE_OS_WINDOWS)
    for(auto &modDir : moduleDirs->getVal()) { addDLLDirectory(as<VarStr>(modDir)->getVal()); }
#endif

    vm.registerType<VarAll>({}, "All", "Base type for all types.");
    vm.registerType<VarNil>({}, "Nil", "Builtin type.");
    vm.registerType<VarBool>({}, "Bool", "Builtin type.");
    vm.registerType<VarInt>({}, "Int", "Builtin type.");
    vm.registerType<VarFlt>({}, "Flt", "Builtin type.");
    vm.registerType<VarStr>({}, "Str", "Builtin type.");
    vm.registerType<VarVec>({}, "Vec", "Builtin type.");
    vm.registerType<VarMap>({}, "Map", "Builtin type.");
    vm.registerType<VarFn>({}, "Func", "Builtin type.");
    vm.registerType<VarModule>({}, "Module", "Builtin type.");
    vm.registerType<VarTypeID>({}, "TypeID", "Builtin type.");
    vm.registerType<VarStructDef>({}, "StructDef", "Builtin type.");
    vm.registerType<VarStruct>({}, "Struct", "Builtin type.");
    vm.registerType<VarFailure>({}, "Failure", "Builtin type.");
    vm.registerType<VarFile>({}, "File", "Builtin type.");
    vm.registerType<VarBytebuffer>({}, "Bytebuffer", "Builtin type.");
    vm.registerType<VarIntIterator>({}, "IntIterator", "Builtin type.");
    vm.registerType<VarVecIterator>({}, "VecIterator", "Builtin type.");
    vm.registerType<VarMapIterator>({}, "MapIterator", "Builtin type.");
    vm.registerType<VarFileIterator>({}, "FileIterator", "Builtin type.");

    return true;
}
bool GlobalState::deinit(VirtualMachine &vm)
{
    using namespace std::chrono_literals;
    vm.stopExecution();
    while(vmCount.load() > 0) { std::this_thread::sleep_for(1ms); }
    for(auto &mod : modules) { vm.decVarRef(mod.second); }
    vm.decVarRef(nil);
    vm.decVarRef(fals);
    vm.decVarRef(tru);
    vm.decVarRef(cmdargs);
    vm.decVarRef(globalModulesPath);
    vm.decVarRef(libPath);
    vm.decVarRef(tempPath);
    vm.decVarRef(installPath);
    vm.decVarRef(binaryPath);
    vm.decVarRef(moduleFinders);
    vm.decVarRef(moduleDirs);
    vm.decVarRef(globals);
    vm.decVarRef(basicErrHandler);
    for(auto &typefn : typefns) { vm.decVarRef(typefn.second); }

#if defined(CORE_OS_WINDOWS)
    remDLLDirectories();
#endif

    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// Other Functions
//////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(CORE_OS_WINDOWS)
bool addDLLDirectory(StringRef dir)
{
    if(dllDirectories.find(dir) != dllDirectories.end()) return true;
    DLL_DIRECTORY_COOKIE dlldir = AddDllDirectory(utils::toWString(dir).c_str());
    if(!dlldir) return false;
    dllDirectories.insert({String(dir), dlldir});
    return true;
}
void remDLLDirectories()
{
    for(auto dir : dllDirectories) { RemoveDllDirectory(dir.second); }
}
#endif

} // namespace fer