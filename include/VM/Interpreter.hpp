#pragma once

#include "Allocator.hpp"
#include "Args.hpp"
#include "Error.hpp"
#include "ExecStack.hpp"
#include "FailStack.hpp"
#include "Vars.hpp"

namespace fer
{

class Interpreter;

typedef bool (*ParseSourceFn)(VirtualMachine &vm, Bytecode &bc, ModuleId moduleId, StringRef path,
			      StringRef code, bool exprOnly);

typedef bool (*ModInitFn)(VirtualMachine &vm, ModuleLoc loc);
typedef void (*ModDeinitFn)(Interpreter &ip);
#define INIT_MODULE(name) extern "C" bool Init##name(VirtualMachine &vm, ModuleLoc loc)
#define DEINIT_MODULE(name) extern "C" void Deinit##name(Interpreter &ip)

class Interpreter
{
	// Used to store VirtualMachine memory after one is free'd
	UniList<VirtualMachine *> freeVMMem;

	ArgParser &argparser;
	ParseSourceFn parseSourceFn;
	MemoryManager mem;
	Map<ModuleId, VarModule *> modules;
	// All functions to call before unloading dlls
	StringMap<ModDeinitFn> dlldeinitfns;
	// Global vars/objects that are required
	VarFrame *globals;
	// Names of types (optional)
	Map<size_t, String> typenames;
	// Functions for all C++ types
	Map<size_t, VarFrame *> typefns;
	// Prelude must be imported before any program is executed
	String prelude;
	// Path where feral binary exists (used by <prelude>.binaryPath)
	String binaryPath;
	Mutex globalMutex;
	// Default dirs to search for modules. Used by basic{Import,Module}Finder()
	VarVec *moduleDirs;
	// Functions (VarVec<VarFn>) to resolve module locations. If one fails, next one is
	// attempted.
	// Signature is: fn(moduleToResolve: str, isImport: bool): nil/str
	VarVec *moduleFinders;
	// Args provided to feral command line
	VarVec *cmdargs;
	VarBool *tru;
	VarBool *fals;
	VarNil *nil;
	// This is the one that's used for checking, and it can be modified by Feral program
	size_t recurseMax;

	friend class VirtualMachine;

	bool loadPrelude();

public:
	Interpreter(ArgParser &argparser, ParseSourceFn parseSourceFn);
	~Interpreter();

	int runFile(ModuleLoc loc, const char *file);

	VirtualMachine *createVM();
	void destroyVM(VirtualMachine *vm);

	// Must be used with full path of directory
	void tryAddModulePathsFromDir(String dir);
	void tryAddModulePathsFromFile(const char *file);

	bool findImportModuleIn(VarVec *dirs, String &name, StringRef srcDir);
	bool findNativeModuleIn(VarVec *dirs, String &name, StringRef srcDir);
	// ext can be empty
	bool findFileIn(VarVec *dirs, String &name, StringRef ext, StringRef srcDir);

	void addGlobal(StringRef name, Var *val, bool iref = true);
	Var *getGlobal(StringRef name);

	void addNativeFn(ModuleLoc loc, StringRef name, NativeFn fn, size_t args,
			 bool is_va = false);
	VarFn *genNativeFn(ModuleLoc loc, StringRef name, NativeFn fn, size_t args,
			   bool is_va = false);

	void addTypeFn(size_t _typeid, StringRef name, Var *fn, bool iref);
	Var *getTypeFn(Var *var, StringRef name);

	void setTypeName(size_t _typeid, StringRef name);
	StringRef getTypeName(size_t _typeid);

	// supposed to call the overloaded delete operator in Var
	Var *getConst(ModuleLoc loc, const Instruction::Data &d, DataType dataty);

	bool hasModule(StringRef path);
	VarModule *getModule(StringRef path);

	void initTypeNames();

	inline const char *getGlobalModulePathsFile() { return GLOBAL_MODULE_PATHS_FILE_PATH; }
	inline StringRef getFeralImportExtension() { return ".fer"; }
	inline StringRef getNativeModuleExtension()
	{
#if defined(FER_OS_WINDOWS)
		return ".dll";
#elif defined(FER_OS_APPLE)
		return ".dylib";
#else
		return ".so";
#endif
	}

	// supposed to call the overloaded new operator in Var
	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type
	makeVarWithRef(Args &&...args)
	{
		return Var::makeVarWithRef<T>(mem, std::forward<Args>(args)...);
	}
	// used in native function calls - sets ref to zero
	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type makeVar(Args &&...args)
	{
		return Var::makeVar<T>(mem, std::forward<Args>(args)...);
	}
	// Generally should be called only by vm.decVarRef(), unless you are sure that var is not
	// being used elsewhere.
	template<typename T>
	typename std::enable_if<std::is_base_of<Var, T>::value, void>::type unmakeVar(T *var)
	{
		return Var::unmakeVar<T>(mem, var);
	}
	template<typename T>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type incVarRef(T *var)
	{
		return Var::incVarRef<T>(var);
	}
	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type
	decVarRef(T *&var, bool del = true)
	{
		return Var::decVarRef<T>(mem, var, del);
	}
	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type
	copyVar(ModuleLoc loc, T *var)
	{
		return Var::copyVar<T>(mem, loc, var);
	}
	template<typename T>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type setVar(T *var, Var *from)
	{
		return Var::setVar<T>(mem, var, from);
	}
	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, void>::type
	registerType(ModuleLoc loc, String name, VarModule *module = nullptr)
	{
		setTypeName(typeID<T>(), name);
		VarTypeID *tyvar = makeVarWithRef<VarTypeID>(loc, typeID<T>());
		name += "Ty";
		if(!module) addGlobal(name, tyvar, false);
		else module->addNativeVar(name, tyvar, false);
	}
};

// Each thread in Interpreter
class VirtualMachine
{
	Interpreter &ip;
	Vars vars;
	Vector<VarModule *> modulestack;
	FailStack failstack;
	ExecStack execstack;
	size_t recurseCount; // how many times execute() has been called by itself
	size_t exitcode;
	bool recurseExceeded;
	bool exitcalled;

public:
	VirtualMachine(Interpreter &ip);
	~VirtualMachine();

	int compileAndRun(ModuleLoc loc, const char *file);
	// Must pushModule before calling this function, and popModule after calling it.
	int execute(bool addFunc = false, bool addBlk = false, size_t begin = 0, size_t end = 0);

	// virtualPath == true for paths like `<eval>` and `<repl>`.
	ModuleId addModule(ModuleLoc loc, StringRef path, String &&code, bool virtualPath,
			   bool exprOnly, VarStack *existingVarStack = nullptr);
	void removeModule(ModuleId moduleId);
	void pushModule(ModuleId moduleId);
	void popModule();

	bool findImportModuleIn(VarVec *dirs, String &name);
	bool findNativeModuleIn(VarVec *dirs, String &name);
	// ext can be empty
	bool findFileIn(VarVec *dirs, String &name, StringRef ext);

	// Here, modpath is the fully resolved module file path
	// moduleStr is the string provided as the argument to loadlib()
	bool loadNativeModule(ModuleLoc loc, const String &modpath, StringRef moduleStr);

	// Used primarily within libraries & by toStr, toBool
	// first arg must ALWAYS be self for memcall, nullptr otherwise
	bool callVar(ModuleLoc loc, StringRef name, Var *&retdata, Span<Var *> args,
		     const StringMap<AssnArgData> &assn_args);
	bool callVar(ModuleLoc loc, StringRef name, Var *callable, Var *&retdata, Span<Var *> args,
		     const StringMap<AssnArgData> &assn_args);
	Var *callVarAndReturn(ModuleLoc loc, StringRef name, Var *callable, Span<Var *> args,
			      const StringMap<AssnArgData> &assn_args);

	// evaluate a given expression and return its result
	// primarily used for templates
	Var *eval(ModuleLoc loc, StringRef code, bool isExpr);

	void dumpExecStack(OStream &os);

	inline void addTypeFn(size_t _typeid, StringRef name, Var *fn, bool iref)
	{
		return ip.addTypeFn(_typeid, name, fn, iref);
	}
	inline Var *getTypeFn(Var *var, StringRef name) { return ip.getTypeFn(var, name); }

	// Must be used with full path of directory
	inline void tryAddModulePathsFromDir(String dir)
	{
		return ip.tryAddModulePathsFromDir(dir);
	}
	inline void tryAddModulePathsFromFile(const char *file)
	{
		return ip.tryAddModulePathsFromFile(file);
	}

	inline void addGlobal(StringRef name, Var *val, bool iref = true)
	{
		return ip.addGlobal(name, val, iref);
	}
	inline Var *getGlobal(StringRef name) { return ip.getGlobal(name); }

	inline void addNativeFn(ModuleLoc loc, StringRef name, NativeFn fn, size_t args,
				bool is_va = false)
	{
		return ip.addNativeFn(loc, name, fn, args, is_va);
	}
	inline VarFn *genNativeFn(ModuleLoc loc, StringRef name, NativeFn fn, size_t args,
				  bool is_va = false)
	{
		return ip.genNativeFn(loc, name, fn, args, is_va);
	}

	inline void setTypeName(size_t _typeid, StringRef name) { ip.setTypeName(_typeid, name); }
	inline StringRef getTypeName(size_t _typeid) { return ip.getTypeName(_typeid); }

	// supposed to call the overloaded delete operator in Var
	inline Var *getConst(ModuleLoc loc, const Instruction::Data &d, DataType dataty)
	{
		return ip.getConst(loc, d, dataty);
	}

	inline Vars &getVars() { return vars; }
	inline void pushExecStack(Var *var, bool iref = true) { execstack.push(var, iref); }
	inline Var *popExecStack(bool dref = true) { return execstack.pop(dref); }
	inline VarModule *getCurrModule() { return modulestack.back(); }
	inline bool isExitCalled() { return exitcalled; }
	inline void setExitCalled(bool called) { exitcalled = called; }
	inline void setExitCode(int exit_code) { exitcode = exit_code; }

	inline Interpreter &getInterpreter() { return ip; }
	inline ArgParser &getArgParser() { return ip.argparser; }
	inline MemoryManager &getMemoryManager() { return ip.mem; }
	inline VarVec *getModuleDirs() { return ip.moduleDirs; }
	inline VarVec *getModuleFinders() { return ip.moduleFinders; }
	inline StringRef getBinaryPath() { return ip.binaryPath; }
	inline VarBool *getTrue() { return ip.tru; }
	inline VarBool *getFalse() { return ip.fals; }
	inline VarNil *getNil() { return ip.nil; }
	inline void setRecurseMax(size_t count) { ip.recurseMax = count; }
	inline size_t getRecurseMax() { return ip.recurseMax; }
	inline VarVec *getCLIArgs() { return ip.cmdargs; }

	inline bool hasModule(StringRef path) { return ip.hasModule(path); }
	inline VarModule *getModule(StringRef path) { return ip.getModule(path); }
	inline StringRef getTypeName(Var *var) { return ip.getTypeName(var->getTypeFnID()); }
	inline const char *getGlobalModulePathsFile() { return ip.getGlobalModulePathsFile(); }
	inline StringRef getFeralImportExtension() { return ip.getFeralImportExtension(); }
	inline StringRef getNativeModuleExtension() { return ip.getNativeModuleExtension(); }

	// supposed to call the overloaded new operator in Var
	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type
	makeVarWithRef(Args &&...args)
	{
		return ip.makeVarWithRef<T>(std::forward<Args>(args)...);
	}
	// used in native function calls - sets ref to zero
	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type makeVar(Args &&...args)
	{
		return ip.makeVar<T>(std::forward<Args>(args)...);
	}
	// Generally should be called only by vm.decVarRef(), unless you are sure that var is not
	// being used elsewhere.
	template<typename T>
	typename std::enable_if<std::is_base_of<Var, T>::value, void>::type unmakeVar(T *var)
	{
		return ip.unmakeVar(var);
	}
	template<typename T>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type incVarRef(T *var)
	{
		return ip.incVarRef(var);
	}
	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type
	decVarRef(T *&var, bool del = true)
	{
		return ip.decVarRef(var, del);
	}
	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type
	copyVar(ModuleLoc loc, T *var)
	{
		return ip.copyVar(loc, var);
	}
	template<typename T>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type setVar(T *var, Var *from)
	{
		return ip.setVar(var, from);
	}

	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, void>::type
	registerType(ModuleLoc loc, String name)
	{
		return ip.registerType<T>(loc, name,
					  modulestack.empty() ? nullptr : modulestack.back());
	}

	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, void>::type
	addNativeTypeFn(ModuleLoc loc, StringRef name, NativeFn fn, size_t args, bool is_va = false)
	{
		VarFn *f =
		makeVarWithRef<VarFn>(loc, modulestack.back()->getModuleId(), "", is_va ? "." : "",
				      args, 0, FnBody{.native = fn}, true);
		for(size_t i = 0; i < args; ++i) f->pushParam("");
		addTypeFn(typeID<T>(), name, f, false);
	}

	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, bool>::type
	callVarAndExpect(ModuleLoc loc, StringRef name, Var *&retdata, Span<Var *> args,
			 const StringMap<AssnArgData> &assn_args)
	{
		if(!callVar(loc, name, retdata, args, assn_args)) return false;
		if(!retdata->is<T>()) {
			fail(loc, "'", name, "' ",
			     !args.empty() && args[0] != nullptr ? "member" : "func",
			     " call expected to return a '", getTypeName(typeID<T>()),
			     "', instead returned: ", getTypeName(retdata));
			decVarRef(retdata);
			return false;
		}
		return true;
	}

	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, bool>::type
	callVarAndExpect(ModuleLoc loc, StringRef name, Var *callable, Var *&retdata,
			 Span<Var *> args, const StringMap<AssnArgData> &assn_args)
	{
		if(!callVar(loc, name, callable, retdata, args, assn_args)) return false;
		if(!retdata->is<T>()) {
			fail(loc, "'", name, "' ",
			     !args.empty() && args[0] != nullptr ? "member" : "func",
			     " call expected to return a '", getTypeName(typeID<T>()),
			     "', instead returned: ", getTypeName(retdata));
			decVarRef(retdata);
			return false;
		}
		return true;
	}

	template<typename... Args> bool fail(ModuleLoc loc, Args &&...args)
	{
		if(failstack.empty() || exitcalled) {
			warn(loc, std::forward<Args>(args)...);
			return false;
		}
		if(failstack.hasErr()) {
			err.fail(loc, std::forward<Args>(args)...);
			return false;
		}
		VarStr *str = makeVarWithRef<VarStr>(loc, "");
		utils::appendToString(str->getVal(), std::forward<Args>(args)...);
		failstack.setErr(str);
		return true;
	}

	// Only shows output if failstack is not in use.
	template<typename... Args> void warn(ModuleLoc loc, Args &&...args)
	{
		if(failstack.empty() || exitcalled) {
			err.fail(loc, std::forward<Args>(args)...);
		}
		// Lose the error msg to the void if failstack is already in place.
	}
};

} // namespace fer