#pragma once

#include "Allocator.hpp"
#include "Args.hpp"
#include "Error.hpp"
#include "ExecStack.hpp"
#include "FailStack.hpp"
#include "Vars.hpp"

namespace fer
{

typedef bool (*ParseSourceFn)(Interpreter &vm, Bytecode &bc, ModuleId moduleId, StringRef path,
			      StringRef code, bool exprOnly);

typedef bool (*ModInitFn)(Interpreter &vm, ModuleLoc loc);
typedef void (*ModDeinitFn)(Interpreter &vm);
#define INIT_MODULE(name) extern "C" bool Init##name(Interpreter &vm, ModuleLoc loc)
#define DEINIT_MODULE(name) extern "C" void Deinit##name(Interpreter &vm)

class Interpreter
{
	ArgParser &argparser;
	ParseSourceFn parseSourceFn;
	MemoryManager mem;
	FailStack failstack;
	ExecStack execstack;
	Map<ModuleId, VarModule *> modules;
	// All functions to call before unloading dlls
	StringMap<ModDeinitFn> dlldeinitfns;
	// Global vars/objects that are required
	VarFrame globals;
	// Names of types (optional)
	Map<size_t, String> typenames;
	// Functions for all C++ types
	Map<size_t, VarFrame *> typefns;
	Vector<VarModule *> modulestack;
	// Prelude must be imported before any program is executed
	String prelude;
	// Path where feral binary exists (used by <prelude>.binaryPath)
	String binaryPath;
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
	size_t recurseCount; // how many times execute() has been called by itself
	size_t exitcode;
	bool recurseExceeded;
	bool exitcalled;

public:
	Interpreter(ArgParser &argparser, ParseSourceFn parseSourceFn);
	~Interpreter();

	int compileAndRun(ModuleLoc loc, const char *file);

	// virtualPath == true for paths like `<eval>` and `<repl>`.
	ModuleId addModule(ModuleLoc loc, StringRef path, String &&code, bool virtualPath,
			   bool exprOnly, Vars *existingVars = nullptr);
	void removeModule(ModuleId moduleId);
	void pushModule(ModuleId moduleId);
	void popModule();

	// Must be used with full path of directory
	void tryAddModulePathsFromDir(String dir);
	void tryAddModulePathsFromFile(const char *file);

	bool findImportModuleIn(VarVec *dirs, String &name);
	bool findNativeModuleIn(VarVec *dirs, String &name);
	// ext can be empty
	bool findFileIn(VarVec *dirs, String &name, StringRef ext);
	// Here, modpath is the fully resolved module file path
	// moduleStr is the string provided as the argument to loadlib()
	bool loadNativeModule(ModuleLoc loc, const String &modpath, StringRef moduleStr);

	void addGlobal(StringRef name, Var *val, bool iref = true);
	inline Var *getGlobal(StringRef name) { return globals.get(name); }

	void addNativeFn(ModuleLoc loc, StringRef name, NativeFn fn, size_t args,
			 bool is_va = false);
	VarFn *genNativeFn(ModuleLoc loc, StringRef name, NativeFn fn, size_t args,
			   bool is_va = false);

	void addTypeFn(size_t _typeid, StringRef name, Var *fn, bool iref);
	Var *getTypeFn(Var *var, StringRef name);

	StringRef getTypeName(size_t _typeid);

	// supposed to call the overloaded delete operator in Var
	Var *getConst(ModuleLoc loc, const Instruction::Data &d, DataType dataty);

	// Must pushModule before calling this function, and popModule after calling it.
	int execute(bool addFunc = true, bool addBlk = false, size_t begin = 0, size_t end = 0);
	// used primarily within libraries & by toStr, toBool
	// first arg must ALWAYS be self for memcall, nullptr otherwise
	bool callVar(ModuleLoc loc, StringRef name, Var *&retdata, Span<Var *> args,
		     const StringMap<AssnArgData> &assn_args);
	bool callVar(ModuleLoc loc, StringRef name, Var *callable, Var *&retdata, Span<Var *> args,
		     const StringMap<AssnArgData> &assn_args);

	// evaluate a given expression and return its result
	// primarily used for templates
	Var *eval(ModuleLoc loc, StringRef code, bool isExpr);

	// Cannot be used to setup functions because the modulestack hasn't been populated at the
	// time of this function's call.
	void initTypeNames();

	void dumpExecStack(OStream &os);

	bool hasModule(StringRef path);
	VarModule *getModule(StringRef path);

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
	registerType(ModuleLoc loc, String name)
	{
		setTypeName(typeID<T>(), name);
		VarTypeID *tyvar = makeVarWithRef<VarTypeID>(loc, typeID<T>());
		name += "Ty";
		if(modulestack.empty()) addGlobal(name, tyvar, false);
		else modulestack.back()->addNativeVar(name, tyvar, false, true);
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

	template<typename... Args> void fail(ModuleLoc loc, Args &&...args)
	{
		if(!failstack.isUsable() || exitcalled) {
			err.fail(loc, std::forward<Args>(args)...);
		} else if(!failstack.getVarName().empty() && !failstack.getErr()) {
			VarStr *str = makeVarWithRef<VarStr>(loc, "");
			utils::appendToString(str->getVal(), std::forward<Args>(args)...);
			failstack.setErr(str);
		}
		// Lose the error msg to the void if the error is handled but not stored in a
		// variable.
	}
	inline ArgParser &getArgParser() { return argparser; }
	inline MemoryManager &getMemoryManager() { return mem; }
	inline void pushExecStack(Var *var, bool iref = true) { execstack.push(var, iref); }
	inline Var *popExecStack(bool dref = true) { return execstack.pop(dref); }
	inline VarModule *getCurrModule() { return modulestack.back(); }
	inline void setTypeName(size_t _typeid, StringRef name) { typenames[_typeid] = name; }
	inline StringRef getTypeName(Var *var) { return getTypeName(var->getTypeFnID()); }
	inline VarVec *getModuleDirs() { return moduleDirs; }
	inline VarVec *getModuleFinders() { return moduleFinders; }
	inline StringRef getBinaryPath() { return binaryPath; }
	inline VarBool *getTrue() { return tru; }
	inline VarBool *getFalse() { return fals; }
	inline VarNil *getNil() { return nil; }
	inline bool isExitCalled() { return exitcalled; }
	inline void setExitCalled(bool called) { exitcalled = called; }
	inline void setExitCode(int exit_code) { exitcode = exit_code; }
	inline void setRecurseMax(size_t count) { recurseMax = count; }
	inline size_t getRecurseMax() { return recurseMax; }
	inline VarVec *getCLIArgs() { return cmdargs; }
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
};

} // namespace fer