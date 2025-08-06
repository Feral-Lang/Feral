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
		const StringMap<AssnArgData> &assn_args);

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
	  globals(VarFrame::create(mem)), prelude("prelude/prelude"),
	  binaryPath(env::getProcPath()), moduleDirs(makeVarWithRef<VarVec>(ModuleLoc(), 2, false)),
	  moduleFinders(makeVarWithRef<VarVec>(ModuleLoc(), 2, false)),
	  tru(makeVarWithRef<VarBool>(ModuleLoc(), true)),
	  fals(makeVarWithRef<VarBool>(ModuleLoc(), false)),
	  nil(makeVarWithRef<VarNil>(ModuleLoc())), recurseMax(DEFAULT_MAX_RECURSE_COUNT)
{
#if defined(CORE_OS_WINDOWS)
	SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
				 LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32 |
				 LOAD_LIBRARY_SEARCH_USER_DIRS);
#endif
	initTypeNames();

	Span<StringRef> vmArgs = argparser.getPassthrough();

	cmdargs = makeVarWithRef<VarVec>(ModuleLoc(), vmArgs.size(), false);
	for(size_t i = 0; i < vmArgs.size(); ++i) {
		auto &a = vmArgs[i];
		cmdargs->push(makeVarWithRef<VarStr>(ModuleLoc(), a));
	}

	VarStr *moduleLoc =
	makeVarWithRef<VarStr>(ModuleLoc(), fs::parentDir(fs::parentDir(binaryPath)));
	moduleLoc->getVal() += "/lib/feral";
	moduleDirs->insert(moduleDirs->begin(), moduleLoc);

	// FERAL_PATHS supercedes the install path, ie. I can even run a custom stdlib if I want :D
	String feral_paths = env::get("FERAL_PATHS");
	for(auto &_path : utils::stringDelim(feral_paths, ";")) {
		VarStr *moduleLoc = makeVarWithRef<VarStr>(ModuleLoc(), _path);
		moduleDirs->insert(moduleDirs->begin(), moduleLoc);
	}

	// Global .modulePaths file.
	// The path of a package is added to it when it's installed from command line via package
	// manager.
	tryAddModulePathsFromFile(getGlobalModulePathsFile());

#if defined(CORE_OS_WINDOWS)
	for(auto &modDir : moduleDirs->getVal()) {
		addDLLDirectory(as<VarStr>(modDir)->getVal());
	}
#endif

	if(!loadPrelude()) throw "Failed to load prelude";
}
Interpreter::~Interpreter()
{
	using namespace std::chrono_literals;
	while(vmCount.load() > 0) {
		std::this_thread::sleep_for(1ms);
	}
	for(auto &mod : modules) {
		decVarRef(mod.second);
	}
	decVarRef(nil);
	decVarRef(fals);
	decVarRef(tru);
	decVarRef(cmdargs);
	decVarRef(moduleFinders);
	decVarRef(moduleDirs);
	for(auto &typefn : typefns) {
		VarFrame::destroy(mem, typefn.second);
	}
	VarFrame::destroy(mem, globals);
	for(auto &deinitfn : dlldeinitfns) {
		deinitfn.second(*this);
	}
	for(auto &item : freeVMMem) {
		mem.free(item);
	}

#if defined(CORE_OS_WINDOWS)
	remDLLDirectories();
#endif
}

bool Interpreter::loadPrelude()
{
	VarFn *bmfFn = genNativeFn({}, "basicModuleFinder", basicModuleFinder, 2);
	moduleFinders->push(bmfFn);
	// loadlib must be setup here because it is needed to load even the core module from
	// <prelude>.
	setupCoreFuncs(*this, {});
	if(!findImportModuleIn(moduleDirs, prelude, fs::getCWD())) {
		err.fail({}, "Failed to find prelude: ", prelude);
		return 1;
	}
	int res = runFile({}, prelude.c_str());
	if(res != 0) {
		err.fail({}, "Failed to import prelude: ", prelude);
		return false;
	}
	// set the prelude/feral global variable
	addGlobal("feral", getModule(prelude));
	return true;
}

VirtualMachine *Interpreter::createVM()
{
	VirtualMachine *vm = nullptr;
	if(!freeVMMem.empty()) {
		vm = new(freeVMMem.front()) VirtualMachine(*this);
		freeVMMem.pop_front();
	}
	if(!vm) vm = mem.alloc<VirtualMachine>(*this);
	return vm;
}
void Interpreter::destroyVM(VirtualMachine *vm)
{
	vm->~VirtualMachine();
	freeVMMem.push_front(vm);
}

int Interpreter::runFile(ModuleLoc loc, const char *file)
{
	VirtualMachine *vm = createVM();
	int res		   = vm->compileAndRun(loc, file);
	destroyVM(vm);
	return res;
}
Var *Interpreter::runCallable(ModuleLoc loc, StringRef name, Var *callable, Span<Var *> args,
			      const StringMap<AssnArgData> &assn_args)
{
	VirtualMachine *vm = createVM();
	Var *res	   = vm->callVarAndReturn(loc, name, callable, args, assn_args);
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

void Interpreter::addGlobal(StringRef name, Var *val, bool iref)
{
	if(globals->exists(name)) return;
	globals->add(name, val, iref);
}
Var *Interpreter::getGlobal(StringRef name) { return globals->get(name); }

void Interpreter::addNativeFn(ModuleLoc loc, StringRef name, NativeFn fn, size_t args, bool is_va)
{
	addGlobal(name, genNativeFn(loc, name, fn, args, is_va), false);
}
VarFn *Interpreter::genNativeFn(ModuleLoc loc, StringRef name, NativeFn fn, size_t args, bool is_va)
{
	VarFn *f =
	makeVarWithRef<VarFn>(loc, -1, "", is_va ? "." : "", args, 0, FnBody{.native = fn}, true);
	for(size_t i = 0; i < args; ++i) f->pushParam("");
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
	auto loc = typefns.find(var->getTypeFnID());
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
	case DataType::FLT: return makeVar<VarFlt>(loc, std::get<long double>(d));
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
	registerType<VarAll>({}, "All");

	registerType<VarNil>({}, "Nil");
	registerType<VarBool>({}, "Bool");
	registerType<VarInt>({}, "Int");
	registerType<VarFlt>({}, "Flt");
	registerType<VarStr>({}, "Str");
	registerType<VarVec>({}, "Vec");
	registerType<VarMap>({}, "Map");
	registerType<VarFn>({}, "Func");
	registerType<VarModule>({}, "Module");
	registerType<VarTypeID>({}, "TypeID");
	registerType<VarStructDef>({}, "StructDef");
	registerType<VarStruct>({}, "Struct");
	registerType<VarFile>({}, "File");
	registerType<VarBytebuffer>({}, "Bytebuffer");
	registerType<VarIntIterator>({}, "IntIterator");
	registerType<VarVecIterator>({}, "VecIterator");
	registerType<VarMapIterator>({}, "MapIterator");
	registerType<VarFileIterator>({}, "FileIterator");

	globals->add("AllTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarAll>()), false);

	globals->add("NilTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarNil>()), false);
	globals->add("BoolTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarBool>()), false);
	globals->add("IntTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarInt>()), false);
	globals->add("FltTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarFlt>()), false);
	globals->add("StrTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarStr>()), false);
	globals->add("VecTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarVec>()), false);
	globals->add("MapTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarMap>()), false);
	globals->add("FuncTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarFn>()), false);
	globals->add("ModuleTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarModule>()),
		     false);
	globals->add("TypeIDTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarTypeID>()),
		     false);
	globals->add("StructDefTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarStructDef>()),
		     false);
	globals->add("StructTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarStruct>()),
		     false);
	globals->add("FileTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarFile>()), false);
	globals->add("BytebufferTy",
		     makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarBytebuffer>()), false);
	globals->add("IntIteratorTy",
		     makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarIntIterator>()), false);
	globals->add("VecIteratorTy",
		     makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarVecIterator>()), false);
	globals->add("MapIteratorTy",
		     makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarMapIterator>()), false);
	globals->add("FileIteratorTy",
		     makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarFileIterator>()), false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// VirtualMachine //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VirtualMachine::VirtualMachine(Interpreter &ip)
	: ip(ip), vars(ip.mem), failstack(ip.mem), execstack(ip.mem), recurseCount(0), exitcode(0),
	  recurseExceeded(false), exitcalled(false)
{
	ip.incVMCount();
}
VirtualMachine::~VirtualMachine() { ip.decVMCount(); }

int VirtualMachine::compileAndRun(ModuleLoc loc, const char *file)
{
	Result<fs::File, bool> readRes = fs::File::create(file, false);
	if(readRes.isErr()) {
		err.fail({}, "Failed to read file: ", file, ": ", readRes.errRef().getMsg());
		return 1;
	}

	ModuleId moduleId = addModule(loc, readRes.val(), false);
	if(moduleId == (ModuleId)-1) {
		err.fail(loc, "Failed to parse module: ", file);
		return 1;
	}

	if(ip.argparser.has("dry")) return 0;

	pushModule(moduleId);
	int res = execute();
	popModule();
	return res;
}

ModuleId VirtualMachine::addModule(ModuleLoc loc, fs::File &&f, bool exprOnly,
				   VarStack *existingVarStack)
{
	static ModuleId moduleIdCtr = 0;
	Bytecode bc;
	if(!ip.parseSourceFn(*this, bc, moduleIdCtr, f, exprOnly)) {
		fail(loc, "failed to parse source: ", f.getPath());
		return -1;
	}
	err.addFile(moduleIdCtr, std::move(f));
	VarModule *mod = makeVarWithRef<VarModule>(loc, err.getPathForId(moduleIdCtr),
						   std::move(bc), moduleIdCtr, existingVarStack);
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

bool VirtualMachine::callVar(ModuleLoc loc, StringRef name, Var *&retdata, Span<Var *> args,
			     const StringMap<AssnArgData> &assn_args)
{
	assert(!modulestack.empty() && "cannot perform a call with empty modulestack");
	bool memcall = args[0] != nullptr;
	Var *fn	     = nullptr;
	if(memcall) {
		if(args[0]->isAttrBased()) fn = args[0]->getAttr(name);
		if(!fn) fn = getTypeFn(args[0], name);
	} else {
		fn = getVars().get(name);
		if(!fn) fn = getGlobal(name);
	}
	if(!fn) {
		if(memcall) {
			fail(loc, "callable '", name,
			     "' does not exist for type: ", getTypeName(args[0]));
		} else {
			fail(loc, "callable '", name, "' does not exist");
		}
		return false;
	}
	if(!fn->isCallable()) {
		fail(loc, "Variable '", name, "' of type '", getTypeName(fn), "' is not callable");
		return false;
	}
	return callVar(loc, name, fn, retdata, args, assn_args);
}

bool VirtualMachine::callVar(ModuleLoc loc, StringRef name, Var *callable, Var *&retdata,
			     Span<Var *> args, const StringMap<AssnArgData> &assn_args)
{
	bool memcall = args[0] != nullptr;
	if(!callable->call(*this, loc, args, assn_args)) {
		if(memcall) {
			fail(loc, "call to '", name, "' failed for type: ", getTypeName(args[0]));
		} else {
			fail(loc, "call to '", name, "' failed");
		}
		return false;
	}
	retdata = popExecStack(false);
	return true;
}
Var *VirtualMachine::callVarAndReturn(ModuleLoc loc, StringRef name, Var *callable,
				      Span<Var *> args, const StringMap<AssnArgData> &assn_args)
{
	Var *retData = nullptr;
	bool res     = callVar(loc, name, callable, retData, args, assn_args);
	if(!res) return nullptr;
	return retData;
}

Var *VirtualMachine::eval(ModuleLoc loc, StringRef code, bool isExpr)
{
	static ModuleId evalCtr = 0;

	Var *res = nullptr;
	int ec	 = 1;

	String path = "<eval.";
	path += utils::toString(evalCtr++);
	path += ">";

	Result<fs::File, bool> fres = fs::File::create(path.c_str(), true);
	fres.valRef().append(code);
	ModuleId moduleId = addModule(loc, fres.val(), isExpr, getVars().getCurrModScope());
	if(moduleId == (ModuleId)-1) {
		fail(loc, "Failed to parse eval code: ", code);
		return nullptr;
	}
	pushModule(moduleId);
	ec = execute();
	popModule();
	if(ec) goto done;
	if(!execstack.empty()) res = popExecStack(false);
	else res = getNil();
done:
	return res;
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
	for(auto dir : dllDirectories) {
		RemoveDllDirectory(dir.second);
	}
}
#endif

} // namespace fer