#pragma once

#include "Allocator.hpp"
#include "Args.hpp"
#include "Error.hpp"
#include "ExecStack.hpp"
#include "FailStack.hpp"
#include "InterpreterState.hpp"
#include "Vars.hpp"

namespace fer
{

class Interpreter;

class InterpreterThread : public IAllocated
{
	Interpreter &interp;
	InterpreterState &globalState;
	Vector<VarModule *> modulestack;
	FailStack failstack;
	ExecStack execstack;
	size_t recurseCount; // how many times execute() has been called by itself
	size_t exitcode;
	bool recurseExceeded;
	bool exitcalled;

public:
	InterpreterThread(Interpreter &interp);
	~InterpreterThread();

	int compileAndRun(ModuleLoc loc, const char *file);

	// virtualPath == true for paths like `<eval>` and `<repl>`.
	ModuleId addModule(ModuleLoc loc, StringRef path, String &&code, bool virtualPath,
			   bool exprOnly, Vars *existingVars = nullptr);
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

	// Must pushModule before calling this function, and popModule after calling it.
	int execute(bool addFunc = true, bool addBlk = false, size_t begin = 0, size_t end = 0);
	// used primarily within libraries & by toStr, toBool
	// first arg must ALWAYS be self for memcall, nullptr otherwise
	bool callVar(ModuleLoc loc, StringRef name, Var *&retdata, Span<Var *> args,
		     const StringMap<AssnArgData> &assn_args);
	bool callVar(ModuleLoc loc, StringRef name, Var *callable, Var *&retdata, Span<Var *> args,
		     const StringMap<AssnArgData> &assn_args);
	Var *callVarThreaded(ModuleLoc loc, String name, Var *callable, Span<Var *> args,
			     const StringMap<AssnArgData> &assn_args);

	// evaluate a given expression and return its result
	// primarily used for templates
	Var *eval(ModuleLoc loc, StringRef code, bool isExpr);

	void dumpExecStack(OStream &os);

	inline void addTypeFn(size_t _typeid, StringRef name, Var *fn, bool iref)
	{
		return globalState.addTypeFn(_typeid, name, fn, iref);
	}
	inline Var *getTypeFn(Var *var, StringRef name) { return globalState.getTypeFn(var, name); }

	// Must be used with full path of directory
	inline void tryAddModulePathsFromDir(String dir)
	{
		return globalState.tryAddModulePathsFromDir(dir);
	}
	inline void tryAddModulePathsFromFile(const char *file)
	{
		return globalState.tryAddModulePathsFromFile(file);
	}

	inline void addGlobal(StringRef name, Var *val, bool iref = true)
	{
		return globalState.addGlobal(name, val, iref);
	}
	inline Var *getGlobal(StringRef name) { return globalState.getGlobal(name); }

	inline void addNativeFn(ModuleLoc loc, StringRef name, NativeFn fn, size_t args,
				bool is_va = false)
	{
		return globalState.addNativeFn(loc, name, fn, args, is_va);
	}
	inline VarFn *genNativeFn(ModuleLoc loc, StringRef name, NativeFn fn, size_t args,
				  bool is_va = false)
	{
		return globalState.genNativeFn(loc, name, fn, args, is_va);
	}

	inline void setTypeName(size_t _typeid, StringRef name)
	{
		globalState.setTypeName(_typeid, name);
	}
	inline StringRef getTypeName(size_t _typeid) { return globalState.getTypeName(_typeid); }

	// supposed to call the overloaded delete operator in Var
	inline Var *getConst(ModuleLoc loc, const Instruction::Data &d, DataType dataty)
	{
		return globalState.getConst(loc, d, dataty);
	}

	inline Interpreter &getInterpreter() { return interp; }
	inline InterpreterState &getGlobalState() { return globalState; }
	inline bool hasModule(StringRef path) { return globalState.hasModule(path); }
	inline VarModule *getModule(StringRef path) { return globalState.getModule(path); }
	inline ArgParser &getArgParser() { return globalState.argparser; }
	inline MemoryManager &getMemoryManager() { return globalState.getMemoryManager(); }
	inline void pushExecStack(Var *var, bool iref = true) { execstack.push(var, iref); }
	inline Var *popExecStack(bool dref = true) { return execstack.pop(dref); }
	inline VarModule *getCurrModule() { return modulestack.back(); }
	inline StringRef getTypeName(Var *var) { return getTypeName(var->getTypeFnID()); }
	inline VarVec *getModuleDirs() { return globalState.moduleDirs; }
	inline VarVec *getModuleFinders() { return globalState.moduleFinders; }
	inline StringRef getBinaryPath() { return globalState.binaryPath; }
	inline VarBool *getTrue() { return globalState.tru; }
	inline VarBool *getFalse() { return globalState.fals; }
	inline VarNil *getNil() { return globalState.nil; }
	inline bool isExitCalled() { return exitcalled; }
	inline void setExitCalled(bool called) { exitcalled = called; }
	inline void setExitCode(int exit_code) { exitcode = exit_code; }
	inline void setRecurseMax(size_t count) { globalState.recurseMax = count; }
	inline size_t getRecurseMax() { return globalState.recurseMax; }
	inline VarVec *getCLIArgs() { return globalState.cmdargs; }
	inline const char *getGlobalModulePathsFile()
	{
		return globalState.getGlobalModulePathsFile();
	}

	inline StringRef getFeralImportExtension() { return globalState.getFeralImportExtension(); }
	inline StringRef getNativeModuleExtension()
	{
		return globalState.getNativeModuleExtension();
	}

	// supposed to call the overloaded new operator in Var
	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type
	makeVarWithRef(Args &&...args)
	{
		return globalState.makeVarWithRef<T>(std::forward<Args>(args)...);
	}
	// used in native function calls - sets ref to zero
	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type makeVar(Args &&...args)
	{
		return globalState.makeVar<T>(std::forward<Args>(args)...);
	}
	// Generally should be called only by vm.decVarRef(), unless you are sure that var is not
	// being used elsewhere.
	template<typename T>
	typename std::enable_if<std::is_base_of<Var, T>::value, void>::type unmakeVar(T *var)
	{
		return globalState.unmakeVar(var);
	}
	template<typename T>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type incVarRef(T *var)
	{
		return globalState.incVarRef(var);
	}
	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type
	decVarRef(T *&var, bool del = true)
	{
		return globalState.decVarRef(var, del);
	}
	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type
	copyVar(ModuleLoc loc, T *var)
	{
		return globalState.copyVar(loc, var);
	}
	template<typename T>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type setVar(T *var, Var *from)
	{
		return globalState.setVar(var, from);
	}

	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, void>::type
	registerType(ModuleLoc loc, String name)
	{
		return globalState.registerType<T>(
		loc, name, modulestack.empty() ? nullptr : modulestack.back());
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
};

} // namespace fer