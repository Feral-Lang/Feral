#pragma once

#include "Allocator.hpp"
#include "Args.hpp"
#include "Error.hpp"
#include "ExecStack.hpp"
#include "FailStack.hpp"
#include "Vars.hpp"

namespace fer
{

typedef bool (*ParseSourceFn)(InterpreterThread &vm, Bytecode &bc, ModuleId moduleId,
			      StringRef path, StringRef code, bool exprOnly);

typedef bool (*ModInitFn)(InterpreterThread &vm, ModuleLoc loc);
typedef void (*ModDeinitFn)(InterpreterState &vms);
#define INIT_MODULE(name) extern "C" bool Init##name(InterpreterThread &vm, ModuleLoc loc)
#define DEINIT_MODULE(name) extern "C" void Deinit##name(InterpreterState &vms)

class InterpreterState
{
public:
	ArgParser &argparser;
	ParseSourceFn parseSourceFn;
	MemoryManager mem;
	Map<ModuleId, VarModule *> modules;
	// All functions to call before unloading dlls
	StringMap<ModDeinitFn> dlldeinitfns;
	// Global vars/objects that are required
	VarFrame globals;
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

	InterpreterState(ArgParser &argparser, ParseSourceFn parseSourceFn);
	~InterpreterState();

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

	inline MemoryManager &getMemoryManager() { return mem; }
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
		T *res = new(mem.alloc(sizeof(T), alignof(T))) T(std::forward<Args>(args)...);
		res->create(*this);
		return res;
	}
	// used in native function calls - sets ref to zero
	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type makeVar(Args &&...args)
	{
		T *res = makeVarWithRef<T>(std::forward<Args>(args)...);
		res->dref();
		return res;
	}
	// Generally should be called only by vm.decVarRef(), unless you are sure that var is not
	// being used elsewhere.
	template<typename T>
	typename std::enable_if<std::is_base_of<Var, T>::value, void>::type unmakeVar(T *var)
	{
		var->destroy(*this);
		var->~T();
		mem.free(var);
	}
	template<typename T>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type incVarRef(T *var)
	{
		if(var == nullptr) return nullptr;
		var->iref();
		return var;
	}
	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type
	decVarRef(T *&var, bool del = true)
	{
		if(var == nullptr) return nullptr;
		var->dref();
		if(del && var->getRef() == 0) {
			unmakeVar(var);
			var = nullptr;
		}
		return var;
	}
	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type
	copyVar(ModuleLoc loc, T *var)
	{
		if(var->isLoadAsRef()) {
			var->unsetLoadAsRef();
			incVarRef(var);
			return var;
		}
		return var->copy(*this, loc);
	}
	template<typename T>
	typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type setVar(T *var, Var *from)
	{
		var->set(*this, from);
		return var;
	}

	template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, void>::type
	registerType(ModuleLoc loc, String name, VarModule *module = nullptr)
	{
		setTypeName(typeID<T>(), name);
		VarTypeID *tyvar = makeVarWithRef<VarTypeID>(loc, typeID<T>());
		name += "Ty";
		if(!module) addGlobal(name, tyvar, false);
		else module->addNativeVar(name, tyvar, false, true);
	}
};

} // namespace fer