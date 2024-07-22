#pragma once

#include "Bytecode.hpp"
#include "Error.hpp"
#include "ExecStack.hpp"
#include "FailStack.hpp"
#include "RAIIParser.hpp"
#include "Utils.hpp"
#include "Vars.hpp"

#if defined(FER_OS_WINDOWS)
// Windows' max recurse count seems to be ~200. So 100 should do.
static constexpr size_t DEFAULT_MAX_RECURSE_COUNT = 100;
#else
static constexpr size_t DEFAULT_MAX_RECURSE_COUNT = 400;
#endif
static const char *MODULE_EXTENSION	   = ".fer";
static const char *MODULE_EXTENSION_NO_DOT = "fer";

namespace fer
{

typedef bool (*ModInitFn)(Interpreter &vm, const ModuleLoc *loc);
typedef void (*ModDeinitFn)();
#define INIT_MODULE(name) extern "C" bool Init##name(Interpreter &vm, const ModuleLoc *loc)
#define DEINIT_MODULE(name) extern "C" void Deinit##name()

// DynLib can be accessed using its static getter (DynLib::getInstance())
// Interpreter should be the parent of all execution threads(?)
class Interpreter
{
	FailStack failstack;
	ExecStack execstack;
	// global vars/objects that are required
	VarFrame globals;
	// functions for all C++ types
	Map<size_t, VarFrame *> typefns;
	// names of types (optional)
	Map<size_t, String> typenames;
	// all functions to call before unloading dlls
	StringMap<ModDeinitFn> dlldeinitfns;
	Map<StringRef, VarModule *> allmodules;
	Vector<VarModule *> modulestack;
	// include and module locations - searches in increasing order of List elements
	Vector<String> includelocs; // should be shared between multiple threads
	Vector<String> dlllocs;	    // should be shared between multiple threads
	// prelude must be imported before any program is executed
	String prelude;
	// path where feral binary exists (used by sys.selfBin())
	String selfbin;
	// parent directory of selfbin (used by sys.selfBase())
	String selfbase;
	// path of the main module
	StringRef mainmodulepath;
	RAIIParser &parser;
	Context &c;
	ArgParser &argparser;
	VarVec *cmdargs; // args provided to feral command line
	VarBool *tru;
	VarBool *fals;
	VarNil *nil;
	size_t exitcode;
	// this is the one that's used for checking, and it can be modified by Feral program
	size_t max_recurse_count;
	size_t recurse_count; // how many times execute() has been called by itself
	bool exitcalled;      // mainly used by sys.exit()
	bool recurse_count_exceeded;

public:
	Interpreter(RAIIParser &parser);
	~Interpreter();

	// supposed to call the overloaded new operator in Var
	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type
	makeVarWithRef(Args &&...args)
	{
		return new T(std::forward<Args>(args)...);
	}
	// used in native function calls - sets ref to zero
	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type makeVar(Args &&...args)
	{
		T *res = makeVarWithRef<T>(std::forward<Args>(args)...);
		res->dref();
		return res;
	}
	template<typename T>
	typename std::enable_if<std::is_base_of<Var, T>::value, void>::type unmakeVar(T *var)
	{
		delete var;
	}

	// compile and run a file; the file argument must be absolute path
	int compileAndRun(const ModuleLoc *loc, String &&file, bool main_module = false);
	inline int compileAndRun(const ModuleLoc *loc, StringRef file, bool main_module = false)
	{
		return compileAndRun(loc, String(file), main_module);
	}

	void addModule(const ModuleLoc *loc, Module *mod, Vars *varsnew = nullptr);
	void removeModule(StringRef path);
	void pushModule(StringRef path);
	void popModule();

	// ext can be empty
	bool findFileIn(Span<String> dirs, String &name, StringRef ext);
	inline bool findImport(String &name)
	{
		return findFileIn(includelocs, name, getFeralImportExtension());
	}
	inline bool findModule(String &name)
	{
		return findFileIn(dlllocs, name, getNativeModuleExtension());
	}
	bool loadNativeModule(const ModuleLoc *loc, String modstr);

	void addGlobal(StringRef name, Var *val, bool iref = true);
	inline Var *getGlobal(StringRef name) { return globals.get(name); }

	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, void>::type
	registerType(const ModuleLoc *loc, String name)
	{
		setTypeName(typeID<T>(), name);
		VarTypeID *tyvar = makeVarWithRef<VarTypeID>(loc, typeID<T>());
		name += "Ty";
		if(modulestack.empty()) addGlobal(name, tyvar, false);
		else modulestack.back()->addNativeVar(name, tyvar, false, true);
	}

	void addNativeFn(const ModuleLoc *loc, StringRef name, NativeFn fn, size_t args,
			 bool is_va = false);

	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, void>::type
	addNativeTypeFn(const ModuleLoc *loc, StringRef name, NativeFn fn, size_t args,
			bool is_va = false)
	{
		VarFn *f =
		makeVarWithRef<VarFn>(loc, modulestack.back()->getMod()->getPath(), "",
				      is_va ? "." : "", args, 0, FnBody{.native = fn}, true);
		for(size_t i = 0; i < args; ++i) f->pushParam("");
		addTypeFn(typeID<T>(), name, f, false);
	}

	void addTypeFn(size_t _typeid, StringRef name, Var *fn, bool iref);
	Var *getTypeFn(Var *var, StringRef name);

	// setTypeName is down (with inline functions)
	StringRef getTypeName(size_t _typeid);

	// supposed to call the overloaded delete operator in Var
	Var *getConst(const ModuleLoc *loc, Instruction::Data &d, DataType dataty);

	// Must pushModule before calling this function, and popModule after calling it.
	int execute(bool addFunc = true, bool addBlk = false, size_t begin = 0, size_t end = 0);
	// used primarily within libraries & by toStr, toBool
	// first arg must ALWAYS be self for memcall, nullptr otherwise
	bool callFn(const ModuleLoc *loc, StringRef name, Var *&retdata, Span<Var *> args,
		    const StringMap<AssnArgData> &assn_args);

	// evaluate a given expression and return its result
	// primarily used for templates
	Var *eval(const ModuleLoc *loc, StringRef code, bool isExpr);

	// Cannot be used to setup functions because the modulestack hasn't been populated at the
	// time of this function's call.
	void initTypeNames();

	void dumpExecStack(OStream &os);

	template<typename... Args> void fail(const ModuleLoc *loc, Args &&...args)
	{
		if(!failstack.isUsable() || exitcalled) {
			err::out(loc, std::forward<Args>(args)...);
		} else if(!failstack.getVarName().empty() && !failstack.getErr()) {
			VarStr *str = makeVarWithRef<VarStr>(loc, "");
			appendToString(str->get(), std::forward<Args>(args)...);
			failstack.setErr(str);
		}
		// Lose the error msg to the void if the error is handled but not stored in a
		// variable.
	}
	inline void pushExecStack(Var *var, bool iref = true) { execstack.push(var, iref); }
	inline Var *popExecStack(bool dref = true) { return execstack.pop(dref); }
	inline bool hasModule(StringRef path) { return allmodules.find(path) != allmodules.end(); }
	inline VarModule *getModule(StringRef path) { return allmodules[path]; }
	inline VarModule *getCurrModule() { return modulestack.back(); }
	inline void setTypeName(size_t _typeid, StringRef name) { typenames[_typeid] = name; }
	inline StringRef getTypeName(Var *var) { return getTypeName(var->getTypeFnID()); }
	inline Span<String> getImportDirs() { return includelocs; }
	inline Span<String> getModuleDirs() { return dlllocs; }
	inline StringRef getSelfBin() { return selfbin; }
	inline StringRef getSelfBase() { return selfbase; }
	inline StringRef getMainModulePath() { return mainmodulepath; }
	inline RAIIParser &getRAIIParser() { return parser; }
	inline Context &getContext() { return c; }
	inline VarBool *getTrue() { return tru; }
	inline VarBool *getFalse() { return fals; }
	inline VarNil *getNil() { return nil; }
	inline bool isExitCalled() { return exitcalled; }
	inline void setExitCalled(bool called) { exitcalled = called; }
	inline void setExitCode(int exit_code) { exitcode = exit_code; }
	inline void setMaxRecurseCount(size_t count) { max_recurse_count = count; }
	inline size_t getMaxRecurseCount() { return max_recurse_count; }
	inline VarVec *getCLIArgs() { return cmdargs; }

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