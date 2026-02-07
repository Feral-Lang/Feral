#include "VM/Interpreter.hpp"

#include "Env.hpp"
#include "Error.hpp"
#include "FS.hpp"
#include "Utils.hpp"
#include "VM/CoreFuncs.hpp"
#include "VM/DynLib.hpp"

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
/////////////////////////////////////// Interpreter //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Interpreter::Interpreter(args::ArgParser &argparser, ParseSourceFn parseSourceFn)
    : vmCount(0), argparser(argparser), parseSourceFn(parseSourceFn), mem("VM::Main"),
      managedAllocator(mem, "VM::ManagedAllocator"), globals(VarFrame::create(mem)),
      prelude("prelude/prelude"),
      binaryPath(makeVarWithRef<VarStr>(ModuleLoc(), env::getProcPath())),
      installPath(makeVarWithRef<VarStr>(ModuleLoc(),
                                         fs::parentDir((fs::parentDir(binaryPath->getVal()))))),
      basicErrHandler(genNativeFn({}, "basicErrorHandler", basicErrorHandler)),
      moduleDirs(makeVarWithRef<VarVec>(ModuleLoc(), 2, false)),
      moduleFinders(makeVarWithRef<VarVec>(ModuleLoc(), 2, false)),
      tru(makeVarWithRef<VarBool>(ModuleLoc(), true)),
      fals(makeVarWithRef<VarBool>(ModuleLoc(), false)), nil(makeVarWithRef<VarNil>(ModuleLoc())),
      recurseMax(DEFAULT_MAX_RECURSE_COUNT)
{
#if defined(CORE_OS_WINDOWS)
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                             LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32 |
                             LOAD_LIBRARY_SEARCH_USER_DIRS);
#endif
    initTypeNames();

    if(installPath->getVal().ends_with(".feral")) {
        tempPath = makeVarWithRef<VarStr>(ModuleLoc(), installPath->getVal() + "/tmp");
        libPath  = makeVarWithRef<VarStr>(ModuleLoc(), installPath->getVal() + "/lib/feral");
        globalModulesPath =
            makeVarWithRef<VarStr>(ModuleLoc(), libPath->getVal() + "/.modulePaths");
    } else {
        // to make sure if user installs Feral in, say, `/usr`, Feral doesn't attempt using
        // `/usr/tmp` as the temp path.
        tempPath = makeVarWithRef<VarStr>(ModuleLoc(), "/tmp/feral");
        libPath  = makeVarWithRef<VarStr>(ModuleLoc(), installPath->getVal() + "/lib/feral");
        globalModulesPath =
            makeVarWithRef<VarStr>(ModuleLoc(), libPath->getVal() + "/.modulePaths");
    }

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

    cmdargs = makeVarWithRef<VarVec>(ModuleLoc(), vmArgs.size(), false);
    for(size_t i = 0; i < vmArgs.size(); ++i) {
        auto &a = vmArgs[i];
        cmdargs->push(makeVarWithRef<VarStr>(ModuleLoc(), a));
    }
    cmdargs->setConst();

    moduleDirs->insert(moduleDirs->begin(), incVarRef(libPath));

    // FERAL_PATHS supercedes the install path, ie. I can even run a custom stdlib if I want :D
    String feralPaths = env::get("FERAL_PATHS");
    for(auto &_path : utils::stringDelim(feralPaths, ";")) {
        VarStr *moduleLoc = makeVarWithRef<VarStr>(ModuleLoc(), _path);
        moduleDirs->insert(moduleDirs->begin(), moduleLoc);
    }

    // Global .modulePaths file.
    // The path of a package is added to it when it's installed from command line via package
    // manager.
    tryAddModulePathsFromFile(globalModulesPath->getVal().c_str());

#if defined(CORE_OS_WINDOWS)
    for(auto &modDir : moduleDirs->getVal()) { addDLLDirectory(as<VarStr>(modDir)->getVal()); }
#endif

    if(!loadPrelude()) throw "Failed to load prelude";
}
Interpreter::~Interpreter()
{
    using namespace std::chrono_literals;
    stopExecution();
    while(vmCount.load() > 0) { std::this_thread::sleep_for(1ms); }
    for(auto &mod : modules) { decVarRef(mod.second); }
    decVarRef(nil);
    decVarRef(fals);
    decVarRef(tru);
    decVarRef(cmdargs);
    decVarRef(moduleFinders);
    decVarRef(moduleDirs);
    decVarRef(basicErrHandler);
    for(auto &typefn : typefns) { VarFrame::destroy(mem, typefn.second); }
    VarFrame::destroy(mem, globals);
    for(auto &deinitfn : dlldeinitfns) { deinitfn.second(*this); }

#if defined(CORE_OS_WINDOWS)
    remDLLDirectories();
#endif
}

bool Interpreter::loadPrelude()
{
    VarFn *bmfFn = genNativeFn({}, "basicModuleFinder", basicModuleFinder);
    moduleFinders->push(bmfFn);
    // loadlib must be setup here because it is needed to load even the core module from
    // <prelude>.
    setupCoreFuncs(*this, {});
    if(!findImportModuleIn(moduleDirs, prelude, fs::getCWD())) {
        err.fail({}, "Failed to find prelude: ", prelude);
        return 1;
    }
    int res = runFile({}, prelude.c_str(), "Main");
    if(res != 0) {
        err.fail({}, "Failed to import prelude: ", prelude);
        return false;
    }
    // set the prelude/feral global variable
    addGlobal("feral", "The feral prelude module.", getModule(prelude));
    return true;
}

VirtualMachine *Interpreter::createVM(StringRef name, VarFn *errHandler, bool iref)
{
    if(!errHandler) errHandler = basicErrHandler;
    return mem.allocInit<VirtualMachine>(*this, name, errHandler, iref);
}
void Interpreter::destroyVM(VirtualMachine *vm) { mem.freeDeinit(vm); }

int Interpreter::runFile(ModuleLoc loc, const char *file, StringRef threadName)
{
    VirtualMachine *vm = createVM(threadName);
    int res            = vm->compileAndRun(loc, file);
    destroyVM(vm);
    return res;
}
Var *Interpreter::runCallable(ModuleLoc loc, StringRef name, Var *callable, Span<Var *> args,
                              const StringMap<AssnArgData> &assnArgs)
{
    VirtualMachine *vm = createVM(name);
    Var *res           = vm->callVar(loc, name, callable, args, assnArgs);
    destroyVM(vm);
    return res;
}

void Interpreter::tryAddModulePathsFromDir(String dir)
{
    // Paths which have already been searched in for the .modulePaths file
    static Set<String> searchedPaths;
    if(searchedPaths.contains(dir)) return;
    searchedPaths.insert(dir);
    String path = dir + "/.modulePaths";
    return tryAddModulePathsFromFile(path.c_str());
}
void Interpreter::tryAddModulePathsFromFile(const char *file)
{
    if(!fs::exists(file)) return;
    String modulePaths;
    if(!fs::read(file, modulePaths).getCode()) return;
    for(auto &_path : utils::stringDelim(modulePaths, "\n")) {
        if(_path.empty()) continue;
        VarStr *moduleLoc = makeVarWithRef<VarStr>(ModuleLoc(), _path);
        moduleDirs->insert(moduleDirs->begin(), moduleLoc);
    }
}

bool Interpreter::findImportModuleIn(VarVec *dirs, String &name, StringRef srcDir)
{
    return findFileIn(dirs, name, getFeralImportExtension(), srcDir);
}
bool Interpreter::findNativeModuleIn(VarVec *dirs, String &name, StringRef srcDir)
{
    name.insert(name.find_last_of('/') + 1, "libferal");
    return findFileIn(dirs, name, getNativeModuleExtension(), srcDir);
}
bool Interpreter::findFileIn(VarVec *dirs, String &name, StringRef ext, StringRef srcDir)
{
    static char testpath[MAX_PATH_CHARS];
    if(name.front() != '~' && name.front() != '/' && name.front() != '.' &&
       (name.size() < 2 || name[1] != ':'))
    {
        for(auto locVar : dirs->getVal()) {
            auto &loc = as<VarStr>(locVar)->getVal();
            strncpy(testpath, loc.data(), loc.size());
            testpath[loc.size()] = '\0';
            strcat(testpath, "/");
            strcat(testpath, name.c_str());
            if(!ext.empty()) strncat(testpath, ext.data(), ext.size());
            if(fs::exists(testpath)) {
                name = fs::absPath(testpath);
                return true;
            }
        }
    } else {
        if(name.front() == '~') {
            name.erase(name.begin());
            static StringRef home = fs::home();
            name.insert(name.begin(), home.begin(), home.end());
        } else if(name.front() == '.' && (name.size() == 1 || name[1] != '.')) {
            assert(srcDir.size() > 0 &&
                   "dot based module search cannot be done on empty modulestack");
            StringRef dir = fs::parentDir(srcDir);
            name.erase(name.begin());
            name.insert(name.begin(), dir.begin(), dir.end());
        } else if(name.size() > 1 && name[0] == '.' && name[1] == '.') {
            assert(srcDir.size() > 0 &&
                   "dot based module search cannot be done on empty modulestack");
            StringRef dir = fs::parentDir(srcDir);
            name.erase(name.begin());
            name.erase(name.begin());
            StringRef parentdir = fs::parentDir(dir);
            name.insert(name.begin(), parentdir.begin(), parentdir.end());
        }
        strcpy(testpath, name.c_str());
        if(!ext.empty()) strncat(testpath, ext.data(), ext.size());
        if(fs::exists(testpath)) {
            name = fs::absPath(testpath);
            return true;
        }
    }
    return false;
}

void Interpreter::addGlobal(StringRef name, StringRef doc, Var *val, bool iref)
{
    if(globals->exists(name)) return;
    if(!doc.empty()) val->setDoc(mem, makeVar<VarStr>(val->getLoc(), doc));
    globals->add(name, val, iref);
}
Var *Interpreter::getGlobal(StringRef name) { return globals->get(name); }

void Interpreter::addNativeFn(ModuleLoc loc, StringRef name, const FeralNativeFnDesc &fnObj)
{
    addGlobal(name, "", genNativeFn(loc, name, fnObj), false);
}
VarFn *Interpreter::genNativeFn(ModuleLoc loc, StringRef name, const FeralNativeFnDesc &fnObj)
{
    VarFn *f = makeVarWithRef<VarFn>(loc, -1, "", fnObj.isVariadic ? "." : "", fnObj.argCount, 0,
                                     FnBody{.native = fnObj.fn}, true);
    if(!fnObj.doc.empty()) f->setDoc(mem, makeVar<VarStr>(loc, fnObj.doc));
    for(size_t i = 0; i < fnObj.argCount; ++i) f->pushParam("");
    return f;
}

void Interpreter::addTypeFn(size_t _typeid, StringRef name, Var *fn, bool iref)
{
    auto loc    = typefns.find(_typeid);
    VarFrame *f = nullptr;
    if(loc == typefns.end()) {
        typefns[_typeid] = f = VarFrame::create(mem);
    } else {
        f = loc->second;
    }
    if(f->exists(name)) {
        err.fail({}, "type function: ", name, " already exists");
        assert(false);
    }
    f->add(name, fn, iref);
}
Var *Interpreter::getTypeFn(Var *var, StringRef name)
{
    auto loc = typefns.find(var->getSubType());
    Var *res = nullptr;
    if(loc != typefns.end()) {
        res = loc->second->get(name);
        if(res) return res;
    } else if(var->isAttrBased()) {
        loc = typefns.find(var->getType());
        if(loc != typefns.end()) {
            res = loc->second->get(name);
            if(res) return res;
        }
    }
    return typefns[typeID<VarAll>()]->get(name);
}

void Interpreter::setTypeName(size_t _typeid, StringRef name) { typenames[_typeid] = name; }
StringRef Interpreter::getTypeName(size_t _typeid)
{
    auto loc = typenames.find(_typeid);
    if(loc == typenames.end()) {
        typenames.insert({_typeid, "typeID<"});
        loc = typenames.find(_typeid);
        loc->second += std::to_string(_typeid);
        loc->second += ">";
    }
    return loc->second;
}

Var *Interpreter::getConst(ModuleLoc loc, const Instruction::Data &d, DataType dataty)
{
    switch(dataty) {
    case DataType::NIL: return nil;
    case DataType::BOOL: return std::get<bool>(d) ? tru : fals;
    case DataType::INT: return makeVar<VarInt>(loc, std::get<int64_t>(d));
    case DataType::FLT: return makeVar<VarFlt>(loc, std::get<double>(d));
    case DataType::STR: return makeVar<VarStr>(loc, std::get<String>(d));
    default: err.fail(loc, "internal error: invalid data type encountered");
    }
    return nullptr;
}

bool Interpreter::hasModule(StringRef path)
{
    for(auto &it : modules) {
        if(it.second->getPath() == path) return true;
    }
    return false;
}
VarModule *Interpreter::getModule(StringRef path)
{
    for(auto &it : modules) {
        if(it.second->getPath() == path) return it.second;
    }
    return nullptr;
}

void Interpreter::initTypeNames()
{
    registerType<VarAll>({}, "All", "Base type for all types.");
    registerType<VarNil>({}, "Nil", "Builtin type.");
    registerType<VarBool>({}, "Bool", "Builtin type.");
    registerType<VarInt>({}, "Int", "Builtin type.");
    registerType<VarFlt>({}, "Flt", "Builtin type.");
    registerType<VarStr>({}, "Str", "Builtin type.");
    registerType<VarVec>({}, "Vec", "Builtin type.");
    registerType<VarMap>({}, "Map", "Builtin type.");
    registerType<VarFn>({}, "Func", "Builtin type.");
    registerType<VarModule>({}, "Module", "Builtin type.");
    registerType<VarTypeID>({}, "TypeID", "Builtin type.");
    registerType<VarStructDef>({}, "StructDef", "Builtin type.");
    registerType<VarStruct>({}, "Struct", "Builtin type.");
    registerType<VarFailure>({}, "Failure", "Builtin type.");
    registerType<VarFile>({}, "File", "Builtin type.");
    registerType<VarBytebuffer>({}, "Bytebuffer", "Builtin type.");
    registerType<VarIntIterator>({}, "IntIterator", "Builtin type.");
    registerType<VarVecIterator>({}, "VecIterator", "Builtin type.");
    registerType<VarMapIterator>({}, "MapIterator", "Builtin type.");
    registerType<VarFileIterator>({}, "FileIterator", "Builtin type.");
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// VirtualMachine //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VirtualMachine::VirtualMachine(Interpreter &ip, StringRef name, VarFn *errHandler, bool iref)
    : ip(ip), name(name), vars(ip.mem), failstack(ip.mem), execstack(ip.mem), recurseCount(0),
      exitcode(0), recurseExceeded(false), exitcalled(false)
{
    // -1 => i will be popLoc - 1, so when ++i happens, with -1 it will be max(size_t)
    failstack.pushHandler(errHandler, -1, 1, iref);
    ip.incVMCount();
}
VirtualMachine::~VirtualMachine()
{
    if(failstack.size() > 0) failstack.popHandler();
    ip.decVMCount();
}

int VirtualMachine::compileAndRun(ModuleLoc loc, const char *file)
{
    fs::File *f          = ip.managedAllocator.alloc<fs::File>(file, false);
    Status<bool> readRes = f->read();
    if(!readRes.getCode()) {
        err.fail({}, "Failed to read file: ", file, ": ", readRes.getMsg());
        return 1;
    }

    ModuleId moduleId = addModule(loc, f, false);
    if(moduleId == (ModuleId)-1) {
        err.fail(loc, "Failed to parse module: ", file);
        return 1;
    }

    if(ip.argparser.has("dry")) return 0;

    pushModule(moduleId);
    Var *ret = nullptr;
    int res  = execute(ret);
    decVarRef(ret);
    popModule();
    return res;
}

ModuleId VirtualMachine::addModule(ModuleLoc loc, fs::File *f, bool exprOnly,
                                   VarStack *existingVarStack)
{
    static ModuleId moduleIdCtr = 0;
    StringRef bcFilePath(f->getPath());
    String bcPath(getTempPath()->getVal());
    bcPath += "/cache";
#if defined(CORE_OS_WINDOWS)
    bcPath += "/";
    auto colonLoc = bcFilePath.find(':');
    if(colonLoc != StringRef::npos) bcFilePath = bcFilePath.substr(colonLoc + 1);
#endif
    bcPath += bcFilePath;
    bcPath += ".bc";
    bool bcFileExists = fs::isFileNewer(bcPath.c_str(), f->getPathCStr());
    Bytecode bc;
    if(bcFileExists) {
        if(!f->isVirtual()) {
            logger.info("Reading bytecode file: ", bcPath);
            FILE *f = fopen(bcPath.c_str(), "rb");
            Bytecode::readFromFile(f, moduleIdCtr, bc);
            fclose(f);
            logger.info("- Read bytecodes: ", bc.size());
        }
    } else {
        if(!ip.parseSourceFn(*this, bc, moduleIdCtr, f->getPath(), f->getData(), exprOnly)) {
            fail(loc, "failed to parse source: ", f->getPath());
            return -1;
        }
        if(!f->isVirtual()) {
            logger.info("Writing bytecode file: ", bcPath);
            std::error_code ec;
            if(fs::mkdir(fs::parentDir(bcPath), ec)) {
                logger.fatal("failed to create directory for bytecode file: ", bcPath,
                             "; error: ", ec.message());
                fail(loc, "failed to create directory for bytecode file: ", bcPath,
                     "; error: ", ec.message());
                return -1;
            }
            FILE *f = fopen(bcPath.c_str(), "wb");
            if(!f) {
                logger.fatal("failed to write bytecode file: ", bcPath);
                fail(loc, "failed to write bytecode file: ", bcPath);
                return -1;
            }
            bc.writeToFile(f);
            fclose(f);
        }
    }
    err.addFile(moduleIdCtr, f);
    VarModule *mod = makeVarWithRef<VarModule>(loc, err.getPathForId(moduleIdCtr), std::move(bc),
                                               moduleIdCtr, existingVarStack);
    LockGuard<Mutex> globalGuard(ip.globalMutex);
    ip.modules.insert_or_assign(moduleIdCtr, mod);
    return moduleIdCtr++;
}
void VirtualMachine::removeModule(ModuleId moduleId)
{
    auto loc = ip.modules.find(moduleId);
    if(loc == ip.modules.end()) return;
    LockGuard<Mutex> globalGuard(ip.globalMutex);
    decVarRef(loc->second);
    ip.modules.erase(loc);
}
void VirtualMachine::pushModule(ModuleId moduleId)
{
    auto mloc = ip.modules.find(moduleId);
    modulestack.push_back(mloc->second);
}
void VirtualMachine::popModule() { modulestack.pop_back(); }

bool VirtualMachine::findImportModuleIn(VarVec *dirs, String &name)
{
    return findFileIn(dirs, name, getFeralImportExtension());
}
bool VirtualMachine::findNativeModuleIn(VarVec *dirs, String &name)
{
    name.insert(name.find_last_of('/') + 1, "libferal");
    return findFileIn(dirs, name, getNativeModuleExtension());
}
bool VirtualMachine::findFileIn(VarVec *dirs, String &name, StringRef ext)
{
    static char testpath[MAX_PATH_CHARS];
    if(name.front() != '~' && name.front() != '/' && name.front() != '.' &&
       (name.size() < 2 || name[1] != ':'))
    {
        for(auto locVar : dirs->getVal()) {
            auto &loc = as<VarStr>(locVar)->getVal();
            strncpy(testpath, loc.data(), loc.size());
            testpath[loc.size()] = '\0';
            strcat(testpath, "/");
            strcat(testpath, name.c_str());
            if(!ext.empty()) strncat(testpath, ext.data(), ext.size());
            if(fs::exists(testpath)) {
                name = fs::absPath(testpath);
                return true;
            }
        }
    } else {
        if(name.front() == '~') {
            name.erase(name.begin());
            static StringRef home = fs::home();
            name.insert(name.begin(), home.begin(), home.end());
        } else if(name.front() == '.' && (name.size() == 1 || name[1] != '.')) {
            assert(modulestack.size() > 0 &&
                   "dot based module search cannot be done on empty modulestack");
            StringRef dir = fs::parentDir(modulestack.back()->getPath());
            name.erase(name.begin());
            name.insert(name.begin(), dir.begin(), dir.end());
        } else if(name.size() > 1 && name[0] == '.' && name[1] == '.') {
            assert(modulestack.size() > 0 &&
                   "dot based module search cannot be done on empty modulestack");
            StringRef dir = fs::parentDir(modulestack.back()->getPath());
            name.erase(name.begin());
            name.erase(name.begin());
            StringRef parentdir = fs::parentDir(dir);
            name.insert(name.begin(), parentdir.begin(), parentdir.end());
        }
        strcpy(testpath, name.c_str());
        if(!ext.empty()) strncat(testpath, ext.data(), ext.size());
        if(fs::exists(testpath)) {
            name = fs::absPath(testpath);
            return true;
        }
    }
    return false;
}

bool VirtualMachine::loadNativeModule(ModuleLoc loc, const String &modpath, StringRef moduleStr)
{
#if defined(CORE_OS_WINDOWS)
    // append the parent dir to dll search paths
    StringRef parentdir = fs::parentDir(modpath);
    if(!addDLLDirectory(parentdir)) {
        fail(loc, "unable to add dir: ", parentdir,
             " as a DLL directory while loading module: ", modpath);
        return false;
    }
#endif

    DynLib &dlibs = DynLib::getInstance();
    if(dlibs.exists(modpath)) return true;

    if(!dlibs.load(modpath.c_str())) {
        fail(loc, "unable to load module file: ", modpath);
        return false;
    }

    StringRef moduleName = moduleStr.substr(moduleStr.find_last_of('/') + 1);

    String tmp = "Init";
    tmp += moduleName;
    ModInitFn initfn = (ModInitFn)dlibs.get(modpath, tmp.c_str());
    if(initfn == nullptr) {
        fail(loc, "unable to load init function '", tmp, "' from module file: ", modpath);
        dlibs.unload(modpath);
        return false;
    }
    if(!initfn(*this, loc)) {
        fail(loc, "init function in module: ", modpath, " failed to execute");
        dlibs.unload(modpath);
        return false;
    }
    // set deinit function if available
    tmp = "Deinit";
    tmp += moduleName;
    ModDeinitFn deinitfn = (ModDeinitFn)dlibs.get(modpath, tmp.c_str());
    if(deinitfn) ip.dlldeinitfns[modpath] = deinitfn;
    return true;
}

Var *VirtualMachine::callVar(ModuleLoc loc, StringRef name, Span<Var *> args,
                             const StringMap<AssnArgData> &assnArgs)
{
    assert(!modulestack.empty() && "cannot perform a call with empty modulestack");
    bool memcall = args[0] != nullptr;
    Var *fn      = nullptr;
    if(memcall) {
        if(args[0]->isAttrBased()) fn = args[0]->getAttr(name);
        if(!fn) fn = getTypeFn(args[0], name);
    } else {
        fn = getVars().get(name);
        if(!fn) fn = getGlobal(name);
    }
    if(!fn) {
        if(memcall) {
            fail(loc, "callable '", name, "' does not exist for type: ", getTypeName(args[0]));
        } else {
            fail(loc, "callable '", name, "' does not exist");
        }
        return nullptr;
    }
    if(!fn->isCallable()) {
        fail(loc, "Variable '", name, "' of type '", getTypeName(fn), "' is not callable");
        return nullptr;
    }
    return callVar(loc, name, fn, args, assnArgs);
}

Var *VirtualMachine::callVar(ModuleLoc loc, StringRef name, Var *callable, Span<Var *> args,
                             const StringMap<AssnArgData> &assnArgs)
{
    bool memcall = args[0] != nullptr;
    Var *retdata = callable->call(*this, loc, args, assnArgs);
    if(!retdata) {
        if(memcall) {
            fail(loc, "call to '", name, "' failed for type: ", getTypeName(args[0]));
        } else {
            fail(loc, "call to '", name, "' failed");
        }
        return nullptr;
    }
    return retdata;
}

Var *VirtualMachine::eval(ModuleLoc loc, StringRef code, bool isExpr)
{
    static ModuleId evalCtr = 0;

    String path = "<eval.";
    path += utils::toString(evalCtr++);
    path += ">";

    fs::File *f = ip.managedAllocator.alloc<fs::File>(path.c_str(), true);
    f->append(code);
    ModuleId moduleId = addModule(loc, f, isExpr, getVars().getCurrModScope());
    if(moduleId == (ModuleId)-1) {
        fail(loc, "Failed to parse eval code: ", code);
        return nullptr;
    }
    pushModule(moduleId);
    Var *tmpRet = nullptr;
    int ec      = execute(tmpRet);
    popModule();
    if(tmpRet) {
        decVarRef(tmpRet);
        fail(loc, "internal error: VirtualMachine::eval() must not generate a return value "
                  "from execute()");
        return nullptr;
    }
    if(ec) return nullptr;
    return execstack.empty() ? incVarRef(getNil()) : popExecStack(false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// Other Functions ///////////////////////////////////////////
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