#include "VM/Interpreter.hpp"

#include "Env.hpp"
#include "Error.hpp"
#include "FS.hpp"
#include "Utils.hpp"
#include "VM/CoreFuncs.hpp"
#include "VM/DynLib.hpp"

#if defined(FER_OS_WINDOWS)
#include <Windows.h> // for libloaderapi.h, which contains AddDllDirectory() and RemoveDllDirectory()
#endif

namespace fer
{

Var *loadModule(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
		const StringMap<AssnArgData> &assn_args);

#if defined(FER_OS_WINDOWS)
static StringMap<DLL_DIRECTORY_COOKIE> dllDirectories;
bool addDLLDirectory(StringRef dir);
void remDLLDirectories();
#endif

Interpreter::Interpreter(ArgParser &argparser, ParseSourceFn parseSourceFn)
	: argparser(argparser), parseSourceFn(parseSourceFn), mem("VM::Main"), failstack(*this),
	  execstack(*this), globals(*this),
	  moduleDirs(makeVarWithRef<VarVec>(ModuleLoc(), 2, false)),
	  moduleFinders(makeVarWithRef<VarVec>(ModuleLoc(), 2, false)), prelude("prelude/prelude"),
	  binaryPath(env::getProcPath()), tru(makeVarWithRef<VarBool>(ModuleLoc(), true)),
	  fals(makeVarWithRef<VarBool>(ModuleLoc(), false)),
	  nil(makeVarWithRef<VarNil>(ModuleLoc())), exitcode(0),
	  max_recurse_count(DEFAULT_MAX_RECURSE_COUNT), recurse_count(0), exitcalled(false),
	  recurse_count_exceeded(false)
{
#if defined(FER_OS_WINDOWS)
	SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
				 LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32 |
				 LOAD_LIBRARY_SEARCH_USER_DIRS);
#endif
	initTypeNames();

	Span<StringRef> vmArgs = argparser.getCodeExecArgs();

	cmdargs = makeVarWithRef<VarVec>(ModuleLoc(), vmArgs.size(), false);
	for(size_t i = 0; i < vmArgs.size(); ++i) {
		auto &a = vmArgs[i];
		cmdargs->push(makeVarWithRef<VarStr>(ModuleLoc(), a));
	}

	String feral_paths = env::get("FERAL_PATHS");
	for(auto &_path : utils::stringDelim(feral_paths, ";")) {
		VarStr *moduleLoc = makeVarWithRef<VarStr>(ModuleLoc(), _path);
		moduleDirs->push(moduleLoc);
	}

	// FERAL_PATHS supercedes the install path, ie. I can even run a custom stdlib if I want :D
	VarStr *moduleLoc = makeVarWithRef<VarStr>(ModuleLoc(), INSTALL_PATH);
	moduleLoc->getVal() += "/lib/feral";
	moduleDirs->push(moduleLoc);

	// Global .modulePaths file.
	// The path of a package is added to it when it's installed from command line via package
	// manager.
	tryAddModulePathsFromFile(getGlobalModulePathsFile());

#if defined(FER_OS_WINDOWS)
	for(auto &modDir : moduleDirs->getVal()) {
		addDLLDirectory(as<VarStr>(modDir)->getVal());
	}
#endif
}

Interpreter::~Interpreter()
{
	decVarRef(nil);
	decVarRef(fals);
	decVarRef(tru);
	decVarRef(cmdargs);
	decVarRef(moduleFinders);
	decVarRef(moduleDirs);
	for(auto &typefn : typefns) {
		delete typefn.second;
	}
	for(auto &deinitfn : dlldeinitfns) {
		deinitfn.second(*this);
	}
	for(auto &mod : modules) decVarRef(mod.second);

#if defined(FER_OS_WINDOWS)
	remDLLDirectories();
#endif
}

int Interpreter::compileAndRun(ModuleLoc loc, const char *file)
{
	static bool firstSource = true;
	String code;

	if(!fs::read(file, code, true)) {
		err.fail(loc, "Failed to read file: ", file);
		return 1;
	}

	ModuleId moduleId = addModule(loc, file, std::move(code), false, false);
	if(moduleId == (ModuleId)-1) {
		err.fail(loc, "Failed to parse module: ", file);
		return 1;
	}

	if(argparser.has("dry")) return 0;

	pushModule(moduleId);
	if(firstSource) {
		firstSource = false;
		moduleFinders->push(genNativeFn({}, "basicModuleFinder", basicModuleFinder, 2));
		setupCoreFuncs(*this, {});
		// loadlib must be setup here because it is needed to load even the core module from
		// <prelude>.
		if(!findImportModuleIn(moduleDirs, prelude)) {
			err.fail(loc, "Failed to find prelude: ", prelude);
			return 1;
		}
		int res = compileAndRun(loc, prelude.c_str());
		if(res != 0) {
			err.fail(loc, "Failed to import prelude: ", prelude);
			popModule();
			removeModule(moduleId);
			return 1;
		}
		// set the prelude/feral global variable
		addGlobal("feral", getModule(prelude));
	}
	int res = execute();
	popModule();
	return res;
}

ModuleId Interpreter::addModule(ModuleLoc loc, StringRef path, String &&code, bool virtualPath,
				bool exprOnly, Vars *existingVars)
{
	static ModuleId moduleIdCtr = 0;
	Bytecode bc;
	if(!parseSourceFn(*this, bc, moduleIdCtr, path, code, exprOnly)) {
		fail(loc, "failed to parse source: ", path);
		return -1;
	}
	err.setPathForId(moduleIdCtr, path);
	if(virtualPath) err.setCodeForId(moduleIdCtr, std::move(code));
	VarModule *mod =
	makeVarWithRef<VarModule>(loc, path, std::move(bc), moduleIdCtr, existingVars);
	modules.insert_or_assign(moduleIdCtr, mod);
	return moduleIdCtr++;
}
void Interpreter::removeModule(ModuleId moduleId)
{
	auto loc = modules.find(moduleId);
	if(loc == modules.end()) return;
	decVarRef(loc->second);
	modules.erase(loc);
}
void Interpreter::pushModule(ModuleId moduleId)
{
	auto mloc = modules.find(moduleId);
	modulestack.push_back(mloc->second);
}
void Interpreter::popModule() { modulestack.pop_back(); }

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
	if(!fs::read(file, modulePaths, true)) return;
	for(auto &_path : utils::stringDelim(modulePaths, "\n")) {
		if(_path.empty()) continue;
		VarStr *moduleLoc = makeVarWithRef<VarStr>(ModuleLoc(), _path);
		moduleDirs->push(moduleLoc);
	}
}

bool Interpreter::findImportModuleIn(VarVec *dirs, String &name)
{
	return findFileIn(dirs, name, getFeralImportExtension());
}
bool Interpreter::findNativeModuleIn(VarVec *dirs, String &name)
{
	name.insert(name.find_last_of('/') + 1, "libferal");
	return findFileIn(dirs, name, getNativeModuleExtension());
}
bool Interpreter::findFileIn(VarVec *dirs, String &name, StringRef ext)
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

bool Interpreter::loadNativeModule(ModuleLoc loc, const String &modpath, StringRef moduleStr)
{
#if defined(FER_OS_WINDOWS)
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
	if(deinitfn) dlldeinitfns[modpath] = deinitfn;
	return true;
}

void Interpreter::addGlobal(StringRef name, Var *val, bool iref)
{
	if(globals.exists(name)) return;
	globals.add(name, val, iref);
}
void Interpreter::addNativeFn(ModuleLoc loc, StringRef name, NativeFn fn, size_t args, bool is_va)
{
	addGlobal(name, genNativeFn(loc, name, fn, args, is_va), false);
}
VarFn *Interpreter::genNativeFn(ModuleLoc loc, StringRef name, NativeFn fn, size_t args, bool is_va)
{
	VarFn *f = makeVarWithRef<VarFn>(loc, modulestack.back()->getModuleId(), "",
					 is_va ? "." : "", args, 0, FnBody{.native = fn}, true);
	for(size_t i = 0; i < args; ++i) f->pushParam("");
	return f;
}
void Interpreter::addTypeFn(size_t _typeid, StringRef name, Var *fn, bool iref)
{
	auto loc    = typefns.find(_typeid);
	VarFrame *f = nullptr;
	if(loc == typefns.end()) {
		typefns[_typeid] = f = new VarFrame(*this);
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

bool Interpreter::callVar(ModuleLoc loc, StringRef name, Var *&retdata, Span<Var *> args,
			  const StringMap<AssnArgData> &assn_args)
{
	assert(!modulestack.empty() && "cannot perform a call with empty modulestack");
	bool memcall = args[0] != nullptr;
	Var *fn	     = nullptr;
	if(memcall) {
		if(args[0]->isAttrBased()) fn = args[0]->getAttr(name);
		if(!fn) fn = getTypeFn(args[0], name);
	} else {
		Vars *vars = getCurrModule()->getVars();
		fn	   = vars->get(name);
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

bool Interpreter::callVar(ModuleLoc loc, StringRef name, Var *callable, Var *&retdata,
			  Span<Var *> args, const StringMap<AssnArgData> &assn_args)
{
	assert(!modulestack.empty() && "cannot perform a call with empty modulestack");
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

Var *Interpreter::eval(ModuleLoc loc, StringRef code, bool isExpr)
{
	static ModuleId evalCtr = 0;

	Var *res = nullptr;
	int ec	 = 1;

	String path = "<eval.";
	path += utils::toString(evalCtr++);
	path += ">";

	ModuleId moduleId =
	addModule(loc, path, String(code), true, isExpr, getCurrModule()->getVars());
	if(moduleId == (ModuleId)-1) {
		fail(loc, "Failed to parse eval code: ", code);
		return nullptr;
	}
	pushModule(moduleId);
	ec = execute(false, false);
	popModule();
	if(ec) goto done;
	if(!execstack.empty()) res = execstack.pop(false);
	else res = getNil();
done:
	return res;
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

	globals.add("AllTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarAll>()), false);

	globals.add("NilTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarNil>()), false);
	globals.add("BoolTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarBool>()), false);
	globals.add("IntTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarInt>()), false);
	globals.add("FltTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarFlt>()), false);
	globals.add("StrTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarStr>()), false);
	globals.add("VecTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarVec>()), false);
	globals.add("MapTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarMap>()), false);
	globals.add("FuncTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarFn>()), false);
	globals.add("ModuleTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarModule>()), false);
	globals.add("TypeIDTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarTypeID>()), false);
	globals.add("StructDefTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarStructDef>()),
		    false);
	globals.add("StructTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarStruct>()), false);
	globals.add("FileTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarFile>()), false);
	globals.add("BytebufferTy", makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarBytebuffer>()),
		    false);
	globals.add("IntIteratorTy",
		    makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarIntIterator>()), false);
	globals.add("VecIteratorTy",
		    makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarVecIterator>()), false);
	globals.add("MapIteratorTy",
		    makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarMapIterator>()), false);
	globals.add("FileIteratorTy",
		    makeVarWithRef<VarTypeID>(ModuleLoc(), typeID<VarFileIterator>()), false);
}

#if defined(FER_OS_WINDOWS)
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