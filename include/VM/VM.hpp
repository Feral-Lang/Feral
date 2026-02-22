#pragma once

#include "ExecStack.hpp"
#include "FailStack.hpp"
#include "GlobalState.hpp"

namespace fer
{

// Perform expressions in variadic before returning nullptr.
#define EXPECT_AND(type, var, expectStr, ...)                                \
    if(!var->is<type>()) {                                                   \
        vm.fail(var->getLoc(), "expected `", vm.getTypeName(typeID<type>()), \
                "` for " expectStr ", found: ", vm.getTypeName(var));        \
        __VA_ARGS__;                                                         \
        return nullptr;                                                      \
    }

#define EXPECT2_AND(type1, type2, var, expectStr, ...)                                  \
    if(!var->is<type1>() && !var->is<type2>()) {                                        \
        vm.fail(var->getLoc(), "expected `", vm.getTypeName(typeID<type1>()), "` or `", \
                vm.getTypeName(typeID<type2>()),                                        \
                "` for " expectStr ", found: ", vm.getTypeName(var));                   \
        __VA_ARGS__;                                                                    \
        return nullptr;                                                                 \
    }

#define EXPECT3_AND(type1, type2, type3, var, expectStr, ...)                               \
    if(!var->is<type1>() && !var->is<type2>() && !var->is<type3>()) {                       \
        vm.fail(var->getLoc(), "expected `", vm.getTypeName(typeID<type1>()), "` or `",     \
                vm.getTypeName(typeID<type2>()), "` or `", vm.getTypeName(typeID<type3>()), \
                "` for " expectStr ", found: ", vm.getTypeName(var));                       \
        __VA_ARGS__;                                                                        \
        return nullptr;                                                                     \
    }

#define EXPECT4_AND(type1, type2, type3, type4, var, expectStr, ...)                        \
    if(!var->is<type1>() && !var->is<type2>() && !var->is<type3>() && !var->is<type4>()) {  \
        vm.fail(var->getLoc(), "expected `", vm.getTypeName(typeID<type1>()), "` or `",     \
                vm.getTypeName(typeID<type2>()), "` or `", vm.getTypeName(typeID<type3>()), \
                "` or `", vm.getTypeName(typeID<type4>()),                                  \
                "` for " expectStr ", found: ", vm.getTypeName(var));                       \
        __VA_ARGS__;                                                                        \
        return nullptr;                                                                     \
    }

#define EXPECT5_AND(type1, type2, type3, type4, type5, var, expectStr, ...)                 \
    if(!var->is<type1>() && !var->is<type2>() && !var->is<type3>() && !var->is<type4>() &&  \
       !var->is<type5>())                                                                   \
    {                                                                                       \
        vm.fail(var->getLoc(), "expected `", vm.getTypeName(typeID<type1>()), "` or `",     \
                vm.getTypeName(typeID<type2>()), "` or `", vm.getTypeName(typeID<type3>()), \
                "` or `", vm.getTypeName(typeID<type4>()), "` or `",                        \
                vm.getTypeName(typeID<type5>()),                                            \
                "` for " expectStr ", found: ", vm.getTypeName(var));                       \
        __VA_ARGS__;                                                                        \
        return nullptr;                                                                     \
    }

#define EXPECT(type, var, expectStr) EXPECT_AND(type, var, expectStr)
#define EXPECT2(type1, type2, var, expectStr) EXPECT2_AND(type1, type2, var, expectStr)
#define EXPECT3(type1, type2, type3, var, expectStr) \
    EXPECT3_AND(type1, type2, type3, var, expectStr)
#define EXPECT4(type1, type2, type3, type4, var, expectStr) \
    EXPECT4_AND(type1, type2, type3, type4, var, expectStr)
#define EXPECT5(type1, type2, type3, type4, type5, var, expectStr) \
    EXPECT5_AND(type1, type2, type3, type4, type5, var, expectStr)

#define EXPECT_NO_CONST(var, name)                    \
    if(var->isConst()) {                              \
        vm.fail(loc, "`" name "` cannot be a const"); \
        return nullptr;                               \
    }

#define EXPECT_ATTR_BASED(var, name)                                                      \
    if(!var->isAttrBased()) {                                                             \
        vm.fail(loc, "`" name "` must be attribute based, found: ", vm.getTypeName(var)); \
        return nullptr;                                                                   \
    }

// Each execution thread. Can spawn more if needed.
class VirtualMachine : public IAllocated
{
    GlobalState *gs;
    String name;
    Vector<VarModule *> modulestack;
    VarVars *vars;
    FailStack *failstack;
    ExecStack *execstack;
    size_t recurseCount; // how many times execute() has been called by itself
    size_t exitcode;
    bool recurseExceeded;
    bool exitCalled;
    bool ownsGlobalState;
    bool ready; // if not ready, _init_() and _deinit_() won't be called

    friend class core::MemoryManager;

    bool loadPrelude();

    VirtualMachine(GlobalState *gs, StringRef name, VarFn *errHandler);

    VirtualMachine *createVM(StringRef name, VarFn *errHandler = nullptr);
    void destroyVM(VirtualMachine *vm);

public:
    VirtualMachine(args::ArgParser &argparser, ParseSourceFn parseSourceFn, StringRef name);
    ~VirtualMachine();

    int runFile(ModuleLoc loc, const char *file, StringRef threadName);
    Var *runCallable(ModuleLoc loc, StringRef name, Var *callable, Span<Var *> args,
                     VarMap *assnArgs);

    int compileAndRun(ModuleLoc loc, const char *file, VarModule **module);
    // Must pushModule before calling this function, and popModule after calling it.
    int execute(Var *&ret, bool addFunc = false, bool addBlk = false, size_t begin = 0,
                size_t end = 0);

    VarModule *makeModule(ModuleLoc loc, File *f, bool exprOnly,
                          VarStack *existingVarStack = nullptr);
    void pushModule(VarModule *module);
    void popModule();

    VarFn *makeFn(ModuleLoc loc, const FeralNativeFnDesc &fnObj);

    void addGlobal(StringRef name, StringRef doc, Var *val, bool iref = true);
    void addGlobal(ModuleLoc loc, StringRef name, const FeralNativeFnDesc &fnObj);
    Var *getGlobal(StringRef name);

    void addLocal(StringRef name, StringRef doc, Var *val, bool iref = true);
    void addLocal(ModuleLoc loc, StringRef name, const FeralNativeFnDesc &fnObj);

    void addTypeFn(size_t _typeid, StringRef name, Var *callable, bool iref);
    Var *getTypeFn(Var *var, StringRef name);

    void setTypeName(size_t _typeid, StringRef name);
    StringRef getTypeName(size_t _typeid);

    // supposed to call the overloaded delete operator in Var
    Var *getConst(ModuleLoc loc, const Instruction::Data &d, DataType dataty);

    bool hasModule(StringRef path);
    VarModule *getModule(StringRef path);

    // Must be used with full path of directory
    void tryAddModulePathsFromDir(String dir);
    void tryAddModulePathsFromFile(const char *file);

    bool findImportIn(VarVec *dirs, String &name, StringRef srcDir = "");
    bool findDllIn(VarVec *dirs, String &name, StringRef srcDir = "");

    // ext can be empty
    bool findFileIn(VarVec *dirs, String &name, StringRef ext, StringRef srcDir);

    // Here, dllpath is the fully resolved dll file path
    // dllStr is the string provided as the argument to loadlib()
    Var *loadDll(ModuleLoc loc, const String &dllpath, StringRef dllStr);

    // Used primarily within libraries & by toStr, toBool
    // first arg must ALWAYS be self for memcall, nullptr otherwise
    Var *callVar(ModuleLoc loc, StringRef name, Span<Var *> args, VarMap *assnArgs);
    Var *callVar(ModuleLoc loc, StringRef name, Var *callable, Span<Var *> args, VarMap *assnArgs);

    // evaluate a given expression and return its result
    // primarily used for templates
    Var *eval(ModuleLoc loc, StringRef code, bool isExpr);

    inline StringRef getName() { return name; }

    inline VarVars *getVars() { return vars; }
    inline VarModule *getCurrModule() { return modulestack.back(); }
    inline bool isExitCalled() { return exitCalled; }
    inline bool isReady() { return ready; }
    inline void setExitCalled(bool called) { exitCalled = called; }
    inline void setExitCode(int code) { exitcode = code; }

    inline GlobalState *getGlobalState() { return gs; }
    inline args::ArgParser &getArgParser() { return gs->argparser; }
    inline MemoryManager &getMemoryManager() { return gs->mem; }
    inline VarVec *getModuleDirs() { return gs->moduleDirs; }
    inline VarVec *getModuleFinders() { return gs->moduleFinders; }
    inline VarPath *getBinaryPath() { return gs->binaryPath; }
    inline VarPath *getInstallPath() { return gs->installPath; }
    inline VarPath *getTempPath() { return gs->tempPath; }
    inline VarPath *getLibPath() { return gs->libPath; }
    inline VarPath *getGlobalModulePathsFile() { return gs->globalModulesPath; }
    inline VarBool *getTrue() { return gs->tru; }
    inline VarBool *getFalse() { return gs->fals; }
    inline VarNil *getNil() { return gs->nil; }
    inline void setRecurseMax(size_t count) { gs->recurseMax = count; }
    inline size_t getRecurseMax() { return gs->recurseMax; }
    inline VarVec *getCLIArgs() { return gs->cmdargs; }

    inline StringRef getTypeName(Var *var) { return getTypeName(var->getSubType()); }

    inline void stopExecution() { gs->stopExec.store(true, std::memory_order_release); }
    inline bool shouldStopExecution() { return gs->stopExec.load(std::memory_order_relaxed); }

    inline StringRef getFeralImportExtension() { return ".fer"; }
    inline StringRef getNativeModuleExtension()
    {
#if defined(CORE_OS_WINDOWS)
        return ".dll";
#elif defined(CORE_OS_APPLE)
        return ".dylib";
#else
        return ".so";
#endif
    }

    template<VarDerived T> T *copyVar(ModuleLoc loc, T *var)
    {
        if(!var) return nullptr;
        return as<T>(var->copy(*this, loc));
    }
    inline bool setVar(Var *dest, Var *src) { return dest->set(*this, src); }

    // makeVar => createVar + initVar
    template<VarDerived T, typename... Args> T *createVar(ModuleLoc loc, Args &&...args)
    {
        T *res = new(gs->mem.allocRaw(sizeof(T), alignof(T))) T(loc, std::forward<Args>(args)...);
        res->create(*this);
        return res;
    }
    // makeVar => createVar + initVar
    template<VarDerived T> T *initVar(T *v)
    {
        v->init(*this);
        return v;
    }

    // makeVar => createVar + initVar
    template<VarDerived T, typename... Args> T *makeVar(ModuleLoc loc, Args &&...args)
    {
        T *res = createVar<T>(loc, std::forward<Args>(args)...);
        return initVar<T>(res);
    }
    template<VarDerived T> T *incVarRef(T *var)
    {
        if(var == nullptr) return nullptr;
        var->iref();
        return var;
    }
    template<VarDerived T> T *decVarRef(T *&var, bool del = true)
    {
        if(var == nullptr) return nullptr;
        if(var->dref() <= 0 && del) {
            var->deinit(*this);
            var->destroy(*this);
            gs->mem.freeDeinit(var);
            var = nullptr;
        }
        return var;
    }

    template<VarDerived T, typename... Args>
    bool makeGlobal(ModuleLoc loc, StringRef name, StringRef doc, Args &&...args)
    {
        T *res = makeVar<T>(loc, std::forward<Args>(args)...);
        if(!res) return false;
        addGlobal(name, doc, res);
        return true;
    }

    template<VarDerived T, typename... Args>
    bool makeLocal(ModuleLoc loc, StringRef name, StringRef doc, Args &&...args)
    {
        T *res = makeVar<T>(loc, std::forward<Args>(args)...);
        if(!res) return false;
        addLocal(name, doc, res);
        return true;
    }

    template<VarDerived T> void addGlobalType(ModuleLoc loc, String name, StringRef doc)
    {
        setTypeName(typeID<T>(), name);
        VarTypeID *tyvar = makeVar<VarTypeID>(loc, typeID<T>());
        tyvar->setConst();
        name += "Ty";
        return addGlobal(name, doc, tyvar);
    }

    template<VarDerived T> void addLocalType(ModuleLoc loc, String name, StringRef doc)
    {
        setTypeName(typeID<T>(), name);
        VarTypeID *tyvar = makeVar<VarTypeID>(loc, typeID<T>());
        tyvar->setConst();
        name += "Ty";
        return addLocal(name, doc, tyvar);
    }

    template<VarDerived T>
    void addTypeFn(ModuleLoc loc, StringRef name, const FeralNativeFnDesc &fnObj)
    {
        VarFn *f = makeFn(loc, fnObj);
        addTypeFn(typeID<T>(), name, f, true);
    }

    template<VarDerived T>
    bool callVarAndExpect(ModuleLoc loc, StringRef name, Var *&retdata, Span<Var *> args,
                          VarMap *assnArgs)
    {
        if(!(retdata = callVar(loc, name, args, assnArgs))) return false;
        if(!retdata->is<T>()) {
            fail(loc, "'", name, "' ", !args.empty() && args[0] != nullptr ? "member" : "func",
                 " call expected to return a '", getTypeName(typeID<T>()),
                 "', instead returned: ", getTypeName(retdata));
            decVarRef(retdata);
            return false;
        }
        return true;
    }

    template<VarDerived T>
    bool callVarAndExpect(ModuleLoc loc, StringRef name, Var *callable, Var *&retdata,
                          Span<Var *> args, VarMap *assnArgs)
    {
        if(!(retdata = callVar(loc, name, callable, args, assnArgs))) return false;
        if(!retdata->is<T>()) {
            fail(loc, "'", name, "' ", !args.empty() && args[0] != nullptr ? "member" : "func",
                 " call expected to return a '", getTypeName(typeID<T>()),
                 "', instead returned: ", getTypeName(retdata));
            decVarRef(retdata);
            return false;
        }
        return true;
    }

    template<typename... Args> void fail(ModuleLoc loc, Args &&...args)
    {
        ready = false;
        return failstack->fail(loc, recurseCount, std::forward<Args>(args)...);
    }
};

} // namespace fer