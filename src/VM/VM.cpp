#include "VM/VM.hpp"

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
//////////////////////////////////// VirtualMachine //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VirtualMachine::VirtualMachine(args::ArgParser &argparser, ParseSourceFn parseSourceFn,
                               StringRef name)
    : gs(new GlobalState(argparser, parseSourceFn)), name(name), recurseCount(0), exitcode(0),
      recurseExceeded(false), exitCalled(false), ownsGlobalState(true)
{
    if(ownsGlobalState && !gs->init(*this)) throw "Failed to initialize GlobalState";
    vars = makeVar<VarVars>({});
    // set vm is ready
    failstack = gs->mem.allocInit<FailStack>(*this);
    execstack = gs->mem.allocInit<ExecStack>(*this);
    // -1 => i will be popLoc - 1, so when ++i happens, with -1 it will be max(size_t)
    failstack->pushHandler(gs->basicErrHandler, -1, 1);
    if(ownsGlobalState && !loadPrelude()) throw "Failed to load prelude module";
    ++gs->vmCount;
}
VirtualMachine::VirtualMachine(GlobalState *gs, StringRef name, VarFn *errHandler)
    : gs(gs), name(name), recurseCount(0), exitcode(0), recurseExceeded(false), exitCalled(false),
      ownsGlobalState(false)
{
    vars      = makeVar<VarVars>({});
    failstack = gs->mem.allocInit<FailStack>(*this);
    execstack = gs->mem.allocInit<ExecStack>(*this);
    if(!errHandler) errHandler = gs->basicErrHandler;
    // -1 => i will be popLoc - 1, so when ++i happens, with -1 it will be max(size_t)
    failstack->pushHandler(errHandler, -1, 1);
    ++gs->vmCount;
}
VirtualMachine::~VirtualMachine()
{
    if(failstack->size() > 0) failstack->popHandler();
    gs->mem.freeDeinit(execstack);
    gs->mem.freeDeinit(failstack);
    decVarRef(vars);
    --gs->vmCount;
    if(ownsGlobalState) {
        gs->deinit(*this);
        delete gs;
    }
}

VirtualMachine *VirtualMachine::createVM(StringRef name, VarFn *errHandler)
{
    return gs->mem.allocInit<VirtualMachine>(gs, name, errHandler);
}
void VirtualMachine::destroyVM(VirtualMachine *vm) { gs->mem.freeDeinit(vm); }

int VirtualMachine::runFile(ModuleLoc loc, const char *file, StringRef threadName)
{
    VirtualMachine *vm = createVM(threadName);
    int res            = vm->compileAndRun(loc, file);
    destroyVM(vm);
    return res;
}
Var *VirtualMachine::runCallable(ModuleLoc loc, StringRef name, Var *callable, Span<Var *> args,
                                 const StringMap<AssnArgData> &assnArgs)
{
    VirtualMachine *vm = createVM(name);
    Var *res           = vm->callVar(loc, name, callable, args, assnArgs);
    destroyVM(vm);
    return res;
}

int VirtualMachine::compileAndRun(ModuleLoc loc, const char *file)
{
    fs::File *f          = gs->managedAllocator.alloc<fs::File>(file, false);
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

    if(gs->argparser.has("dry")) return 0;

    pushModule(moduleId);
    Var *ret = nullptr;
    int res  = execute(ret);
    decVarRef(ret);
    popModule();
    return res;
}

bool VirtualMachine::loadPrelude()
{
    addGlobal({}, "import", loadFile);
    addGlobal({}, "loadlib", loadLibrary);

    VarFn *bmfFn = makeFn({}, basicModuleFinder);
    gs->moduleFinders->push(*this, bmfFn, true);
    // loadlib must be setup here because it is needed to load even the core module from
    // <prelude>.

    if(!findImportIn(gs->moduleDirs, gs->prelude, fs::getCWD())) {
        err.fail({}, "Failed to find prelude: ", gs->prelude);
        return 1;
    }
    int res = compileAndRun({}, gs->prelude.c_str());
    if(res != 0) {
        err.fail({}, "Failed to import prelude: ", gs->prelude);
        return false;
    }
    // set the prelude/feral global variable
    addGlobal("feral", "The feral prelude module.", getModule(gs->prelude));
    return true;
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
#endif
    bcPath += bcFilePath;
    bcPath += ".bc";
#if defined(CORE_OS_WINDOWS)
    utils::stringReplace(bcPath, ":", "/");
#endif
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
        if(!gs->parseSourceFn(*this, bc, moduleIdCtr, f->getPath(), f->getData(), exprOnly)) {
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
    VarModule *mod = incVarRef(makeVar<VarModule>(loc, err.getPathForId(moduleIdCtr), std::move(bc),
                                                  moduleIdCtr, existingVarStack));
    LockGuard<RecursiveMutex> globalGuard(gs->mutex);
    gs->modules.insert_or_assign(moduleIdCtr, mod);
    return moduleIdCtr++;
}
void VirtualMachine::removeModule(ModuleId moduleId)
{
    auto loc = gs->modules.find(moduleId);
    if(loc == gs->modules.end()) return;
    LockGuard<RecursiveMutex> globalGuard(gs->mutex);
    decVarRef(loc->second);
    gs->modules.erase(loc);
}
void VirtualMachine::pushModule(ModuleId moduleId)
{
    auto mloc = gs->modules.find(moduleId);
    modulestack.push_back(mloc->second);
}
void VirtualMachine::popModule() { modulestack.pop_back(); }

VarFn *VirtualMachine::makeFn(ModuleLoc loc, const FeralNativeFnDesc &fnObj)
{
    VarFn *f = makeVar<VarFn>(loc, -1, "", fnObj.isVariadic ? "." : "", fnObj.argCount, 0,
                              FnBody{.native = fnObj.fn}, true);
    if(!f) return nullptr;
    if(!fnObj.doc.empty()) f->setDoc(*this, loc, fnObj.doc);
    for(size_t i = 0; i < fnObj.argCount; ++i) f->pushParam("");
    return f;
}

void VirtualMachine::addGlobal(StringRef name, StringRef doc, Var *val, bool iref)
{
    if(!doc.empty()) val->setDoc(*this, val->getLoc(), doc);
    gs->globals->setAttr(*this, name, val, iref);
}
void VirtualMachine::addGlobal(ModuleLoc loc, StringRef name, const FeralNativeFnDesc &fnObj)
{
    addGlobal(name, "", makeFn(loc, fnObj));
}
Var *VirtualMachine::getGlobal(StringRef name) { return gs->globals->getAttr(name); }

void VirtualMachine::addLocal(StringRef name, StringRef doc, Var *val, bool iref)
{
    VarModule *mod = getCurrModule();
    if(!doc.empty()) val->setDoc(*this, val->getLoc(), doc);
    mod->setAttr(*this, name, val, iref);
}
void VirtualMachine::addLocal(ModuleLoc loc, StringRef name, const FeralNativeFnDesc &fnObj)
{
    addLocal(name, "", makeFn(loc, fnObj));
}

void VirtualMachine::addTypeFn(size_t _typeid, StringRef name, Var *callable, bool iref)
{
    auto loc  = gs->typefns.find(_typeid);
    VarMap *f = nullptr;
    if(loc == gs->typefns.end()) {
        gs->typefns[_typeid] = f = incVarRef(makeVar<VarMap>({}, 0, false));
    } else {
        f = loc->second;
    }
    f->setAttr(*this, name, callable, iref);
}
Var *VirtualMachine::getTypeFn(Var *var, StringRef name)
{
    auto loc = gs->typefns.find(var->getSubType());
    Var *res = nullptr;
    if(loc != gs->typefns.end()) {
        res = loc->second->getAttr(name);
        if(res) return res;
    }
    loc = gs->typefns.find(var->getType());
    if(loc != gs->typefns.end()) {
        res = loc->second->getAttr(name);
        if(res) return res;
    }
    return gs->typefns[typeID<VarAll>()]->getAttr(name);
}

void VirtualMachine::setTypeName(size_t _typeid, StringRef name) { gs->typenames[_typeid] = name; }
StringRef VirtualMachine::getTypeName(size_t _typeid)
{
    auto loc = gs->typenames.find(_typeid);
    if(loc == gs->typenames.end()) {
        gs->typenames.insert({_typeid, "typeID<"});
        loc = gs->typenames.find(_typeid);
        loc->second += std::to_string(_typeid);
        loc->second += ">";
    }
    return loc->second;
}

Var *VirtualMachine::getConst(ModuleLoc loc, const Instruction::Data &d, DataType dataty)
{
    switch(dataty) {
    case DataType::NIL: return gs->nil;
    case DataType::BOOL: return std::get<bool>(d) ? gs->tru : gs->fals;
    case DataType::INT: return makeVar<VarInt>(loc, std::get<int64_t>(d));
    case DataType::FLT: return makeVar<VarFlt>(loc, std::get<double>(d));
    case DataType::STR: return makeVar<VarStr>(loc, std::get<String>(d));
    default: err.fail(loc, "internal error: invalid data type encountered");
    }
    return nullptr;
}

bool VirtualMachine::hasModule(StringRef path)
{
    for(auto &it : gs->modules) {
        if(it.second->getPath() == path) return true;
    }
    return false;
}
VarModule *VirtualMachine::getModule(StringRef path)
{
    for(auto &it : gs->modules) {
        if(it.second->getPath() == path) return it.second;
    }
    return nullptr;
}

void VirtualMachine::tryAddModulePathsFromDir(String dir)
{
    // Paths which have already been searched in for the .modulePaths file
    static Set<String> searchedPaths;
    if(searchedPaths.contains(dir)) return;
    searchedPaths.insert(dir);
    String path = dir + "/.modulePaths";
    return tryAddModulePathsFromFile(path.c_str());
}
void VirtualMachine::tryAddModulePathsFromFile(const char *file)
{
    if(!fs::exists(file)) return;
    String modulePaths;
    if(!fs::read(file, modulePaths).getCode()) return;
    for(auto &_path : utils::stringDelim(modulePaths, "\n")) {
        if(_path.empty()) continue;
        VarStr *moduleLoc = makeVar<VarStr>({}, _path);
        gs->moduleDirs->insert(*this, 0, moduleLoc, true);
    }
}

bool VirtualMachine::findImportIn(VarVec *dirs, String &name, StringRef srcDir)
{
    if(srcDir.empty()) {
        assert(modulestack.size() > 0 &&
               "dot based module search cannot be done on empty modulestack");
        srcDir = modulestack.back()->getPath();
    }
    return findFileIn(dirs, name, getFeralImportExtension(), srcDir);
}
bool VirtualMachine::findDllIn(VarVec *dirs, String &name, StringRef srcDir)
{
    if(srcDir.empty()) {
        assert(modulestack.size() > 0 &&
               "dot based module search cannot be done on empty modulestack");
        srcDir = modulestack.back()->getPath();
    }
    name.insert(name.find_last_of('/') + 1, "libferal");
    return findFileIn(dirs, name, getNativeModuleExtension(), srcDir);
}
bool VirtualMachine::findFileIn(VarVec *dirs, String &name, StringRef ext, StringRef srcDir)
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

Var *VirtualMachine::loadDll(ModuleLoc loc, const String &dllpath, StringRef dllStr)
{
#if defined(CORE_OS_WINDOWS)
    // append the parent dir to dll search paths
    StringRef parentdir = fs::parentDir(modpath);
    if(!addDLLDirectory(parentdir)) {
        fail(loc, "unable to add dir: ", parentdir,
             " as a DLL directory while loading module: ", modpath);
        return nullptr;
    }
#endif

    DynLib &dlibs = DynLib::getInstance();
    if(dlibs.exists(dllpath)) return getNil();

    if(!dlibs.load(dllpath.c_str())) {
        fail(loc, "unable to load module file: ", dllpath);
        return nullptr;
    }

    StringRef dllName = dllStr.substr(dllStr.find_last_of('/') + 1);

    String tmp = "Init";
    tmp += dllName;
    DllInitFn initfn = (DllInitFn)dlibs.get(dllpath, tmp.c_str());
    if(initfn == nullptr) {
        fail(loc, "unable to load init function '", tmp, "' from module file: ", dllpath);
        dlibs.unload(dllpath);
        return nullptr;
    }
    if(!initfn(*this, loc)) {
        fail(loc, "init function in module: ", dllpath, " failed to execute");
        dlibs.unload(dllpath);
        return nullptr;
    }
    // set deinit function if available
    tmp = "Deinit";
    tmp += dllName;
    DllDeinitFn deinitfn = (DllDeinitFn)dlibs.get(dllpath, tmp.c_str());
    return makeVar<VarDll>(loc, initfn, deinitfn);
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
        fn = getVars()->getAttr(name);
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

    fs::File *f = gs->managedAllocator.alloc<fs::File>(path.c_str(), true);
    f->append(code);
    ModuleId moduleId = addModule(loc, f, isExpr, getVars()->getCurrModScope());
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
    return execstack->empty() ? incVarRef(getNil()) : popExecStack(false);
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