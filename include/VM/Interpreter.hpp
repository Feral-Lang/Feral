#pragma once

#include "Args.hpp"
#include "Bytecode.hpp"
#include "Context.hpp"
#include "ExecStack.hpp"
#include "FailStack.hpp"
#include "VarMemory.hpp"
#include "Vars.hpp"

static constexpr size_t DEFAULT_MAX_RECURSE_COUNT = 400;
static const char *MODULE_EXTENSION		  = ".fer";
static const char *MODULE_EXTENSION_NO_DOT	  = "fer";

namespace fer
{

// this is the one that's used for checking, and it can be modified by Feral program
extern size_t MAX_RECURSE_COUNT;

typedef bool (*ModInitFn)(Interpreter &interp, const ModuleLoc *loc);
typedef void (*ModDeinitFn)();
#define INIT_MODULE(name) extern "C" bool Init##name(Interpreter &interp, const ModuleLoc *loc)
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
	Map<uiptr, VarFrame *> typefns;
	// names of types (optional)
	Map<uiptr, StringRef> typenames;
	// all functions to call before unloading dlls
	Map<StringRef, ModDeinitFn> dlldeinitfns;
	Map<StringRef, VarModule *> allmodules;
	Vector<VarModule *> modulestack;
	// include and module locations - searches in increasing order of List elements
	Vector<StringRef> includelocs; // should be shared between multiple threads
	Vector<StringRef> dlllocs;     // should be shared between multiple threads
	// path where feral binary exists (used by sys.selfBin())
	StringRef selfbin;
	// parent directory of selfbin (used by sys.selfBase())
	StringRef selfbase;
	Context &c;
	ArgParser &argparser;
	VarVec *cmdargs; // args provided to feral command line
	VarBool *tru;
	VarBool *fals;
	VarNil *nil;
	size_t exitcode;
	size_t recurse_count; // how many times execute() has been called by itself
	bool exit_called;     // mainly used by sys.exit()
	bool recurse_count_exceeded;

public:
	Interpreter(Context &c, ArgParser &argparser);
	~Interpreter();

	// supposed to call the overloaded new operator in Var
	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type makeVar(Args &&...args)
	{
		return new T(std::forward<Args>(args)...);
	}
	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type
	makeVarDref(Args &&...args)
	{
		T *res = makeVar<T>(std::forward<Args>(args)...);
		res->dref();
		return res;
	}

	void pushModule(const ModuleLoc *loc, Module *mod);
	void pushModule(StringRef path);
	void popModule();

	bool moduleExists(Span<StringRef> locs, String &mod);
	bool loadNativeModule(const ModuleLoc *loc, StringRef modstr);

	void addGlobal(StringRef name, Var *val, bool iref = true);
	inline Var *getGlobal(StringRef name) { return globals.get(name); }

	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, void>::type
	registerType(StringRef _name, const ModuleLoc *loc = nullptr)
	{
		setTypeName(typeID<T>(), _name);
		VarTypeID *tyvar = makeVar<VarTypeID>(loc, typeID<T>());
		StringRef name	 = c.strFrom({_name, "Ty"});
		if(modulestack.empty()) addGlobal(name, tyvar, false);
		else modulestack.back()->addNativeVar(name, tyvar, false, true);
	}

	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, void>::type
	addNativeTypeFn(const ModuleLoc *loc, StringRef name, NativeFn fn, size_t args,
			bool is_va = false)
	{
		VarFn *f = makeVar<VarFn>(loc, modulestack.back()->getMod()->getPath(), "",
					  is_va ? "." : "", args, 0, FnBody{.native = fn}, true);
		for(size_t i = 0; i < args; ++i) f->pushParam("");
		addTypeFn(typeID<T>(), name, f, false);
	}

	void addTypeFn(uiptr _typeid, StringRef name, Var *fn, bool iref);
	Var *getTypeFn(Var *var, StringRef name);

	// setTypeName is down (with inline functions)
	StringRef getTypeName(uiptr _typeid);

	// supposed to call the overloaded delete operator in Var
	Var *getConst(const ModuleLoc *loc, Data &d, DataType dataty);

	int execute(Bytecode *custombc = nullptr, size_t begin = 0, size_t end = 0);

	void initTypeNames();

	void fail(const ModuleLoc *loc, InitList<StringRef> err);
	inline void pushExecStack(Var *var, bool iref = true) { execstack.push(var, iref); }
	inline Var *popExecStack(bool dref = true) { return execstack.pop(dref); }
	inline VarModule *getCurrModule() { return modulestack.back(); }
	inline void setTypeName(uiptr _typeid, StringRef name) { typenames[_typeid] = name; }
	inline StringRef getTypeName(Var *var) { return getTypeName(var->getType()); }
	inline StringRef getSelfBin() { return selfbin; }
	inline StringRef getSelfBase() { return selfbase; }
	inline Context &getContext() { return c; }
	inline VarBool *getTrue() { return tru; }
	inline VarBool *getFalse() { return fals; }
	inline VarNil *getNil() { return nil; }

	StringRef getNativeModuleExtension()
	{
#if __linux__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__ || __bsdi__ || __DragonFly__
		return ".so";
#elif __APPLE__
		return ".dylib";
#endif
	}
};

} // namespace fer