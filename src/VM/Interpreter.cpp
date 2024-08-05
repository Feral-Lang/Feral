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

Var *loadModule(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		const StringMap<AssnArgData> &assn_args);

#if defined(FER_OS_WINDOWS)
static StringMap<DLL_DIRECTORY_COOKIE> dllDirectories;
bool addDLLDirectory(StringRef dir);
void remDLLDirectories();
#endif

Interpreter::Interpreter(RAIIParser &parser)
	: mem("VM::Main"), failstack(*this), execstack(*this), globals(*this),
	  moduleDirs(makeVarWithRef<VarVec>(nullptr, 2, false)),
	  moduleFinders(makeVarWithRef<VarVec>(nullptr, 2, false)), prelude("prelude/prelude"),
	  binaryPath(env::getProcPath()), parser(parser), c(parser.getContext()),
	  argparser(parser.getCommandArgs()), tru(makeVarWithRef<VarBool>(nullptr, true)),
	  fals(makeVarWithRef<VarBool>(nullptr, false)), nil(makeVarWithRef<VarNil>(nullptr)),
	  exitcode(0), max_recurse_count(DEFAULT_MAX_RECURSE_COUNT), recurse_count(0),
	  exitcalled(false), recurse_count_exceeded(false)
{
#if defined(FER_OS_WINDOWS)
	SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
				 LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32 |
				 LOAD_LIBRARY_SEARCH_USER_DIRS);
#endif
	initTypeNames();

	Span<StringRef> _cmdargs = argparser.getCodeExecArgs();
	cmdargs			 = makeVarWithRef<VarVec>(nullptr, _cmdargs.size(), false);
	for(size_t i = 0; i < _cmdargs.size(); ++i) {
		auto &a = _cmdargs[i];
		cmdargs->push(makeVarWithRef<VarStr>(nullptr, a));
	}

	String feral_paths = env::get("FERAL_PATHS");
	for(auto &_path : stringDelim(feral_paths, ";")) {
		VarStr *moduleLoc = makeVarWithRef<VarStr>(nullptr, _path);
		moduleDirs->push(moduleLoc);
	}

	// FERAL_PATHS supercedes the install path, ie. I can even run a custom stdlib if I want :D
	VarStr *moduleLoc = makeVarWithRef<VarStr>(nullptr, INSTALL_PATH);
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
		deinitfn.second();
	}
	for(auto &mod : allmodules) decVarRef(mod.second);

#if defined(FER_OS_WINDOWS)
	remDLLDirectories();
#endif
}

int Interpreter::compileAndRun(const ModuleLoc *loc, String &&file, bool main_module)
{
	Module *mod = parser.createModule(std::move(file), main_module);

	if(!mod) return 1;
	if(!mod->tokenize()) return 1;
	if(argparser.has("lex")) mod->dumpTokens();
	if(!mod->parseTokens()) return 1;
	if(argparser.has("parse")) mod->dumpParseTree();
	if(!mod->executeDefaultPasses()) {
		err::out(loc, "Failed to apply default parser passes on module: ", mod->getPath());
		return 1;
	}
	if(argparser.has("optparse")) mod->dumpParseTree();
	if(!mod->genCode()) return 1;
	if(argparser.has("ir")) mod->dumpCode();
	if(argparser.has("dry")) return 0;

	addModule(loc, mod);

	pushModule(mod->getPath());
	if(main_module) {
		moduleFinders->push(
		genNativeFn(nullptr, "basicModuleFinder", basicModuleFinder, 2));
		setupCoreFuncs(*this, nullptr);
		// loadlib must be setup here because it is needed to load even the core module from
		// <prelude>.
		if(!findImportModuleIn(moduleDirs, prelude)) {
			err::out(loc, "Failed to find prelude: ", prelude);
			return 1;
		}
		int res = compileAndRun(loc, prelude, false);
		if(res != 0) {
			err::out(loc, "Failed to import prelude: ", prelude);
			removeModule(mod->getPath());
			popModule();
			return 1;
		}
		// set the prelude/feral global variable
		addGlobal("feral", getModule(prelude));
		mainmodulepath = mod->getPath();
	}
	int res = execute();
	popModule();
	return res;
}

void Interpreter::addModule(const ModuleLoc *loc, Module *mod, Vars *varsnew)
{
	auto l = allmodules.find(mod->getPath());
	if(l != allmodules.end()) decVarRef(l->second);
	allmodules[mod->getPath()] = makeVarWithRef<VarModule>(loc, mod, varsnew, !varsnew);
}
void Interpreter::removeModule(StringRef path)
{
	auto loc = allmodules.find(path);
	if(loc == allmodules.end()) return;
	decVarRef(loc->second);
	allmodules.erase(loc);
}
void Interpreter::pushModule(StringRef path)
{
	auto mloc = allmodules.find(path);
	incVarRef(mloc->second);
	modulestack.push_back(mloc->second);
}
void Interpreter::popModule()
{
	decVarRef(modulestack.back());
	modulestack.pop_back();
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
	if(!fs::read(file, modulePaths, true)) return;
	for(auto &_path : stringDelim(modulePaths, "\n")) {
		VarStr *moduleLoc = makeVarWithRef<VarStr>(nullptr, _path);
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
			StringRef dir = modulestack.back()->getMod()->getDir();
			name.erase(name.begin());
			name.insert(name.begin(), dir.begin(), dir.end());
		} else if(name.size() > 1 && name[0] == '.' && name[1] == '.') {
			assert(modulestack.size() > 0 &&
			       "dot based module search cannot be done on empty modulestack");
			StringRef dir = modulestack.back()->getMod()->getDir();
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

bool Interpreter::loadNativeModule(const ModuleLoc *loc, const String &modpath, StringRef moduleStr)
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
void Interpreter::addNativeFn(const ModuleLoc *loc, StringRef name, NativeFn fn, size_t args,
			      bool is_va)
{
	addGlobal(name, genNativeFn(loc, name, fn, args, is_va), false);
}
VarFn *Interpreter::genNativeFn(const ModuleLoc *loc, StringRef name, NativeFn fn, size_t args,
				bool is_va)
{
	VarFn *f = makeVarWithRef<VarFn>(loc, modulestack.back()->getMod()->getPath(), "",
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
		err::out(nullptr, "type function: ", name, " already exists");
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

Var *Interpreter::getConst(const ModuleLoc *loc, Instruction::Data &d, DataType dataty)
{
	switch(dataty) {
	case DataType::NIL: return nil;
	case DataType::BOOL: return std::get<bool>(d) ? tru : fals;
	case DataType::INT: return makeVar<VarInt>(loc, std::get<int64_t>(d));
	case DataType::FLT: return makeVar<VarFlt>(loc, std::get<long double>(d));
	case DataType::STR: return makeVar<VarStr>(loc, std::get<String>(d));
	default: err::out(loc, "internal error: invalid data type encountered");
	}
	return nullptr;
}

bool Interpreter::callVar(const ModuleLoc *loc, StringRef name, Var *&retdata, Span<Var *> args,
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

bool Interpreter::callVar(const ModuleLoc *loc, StringRef name, Var *callable, Var *&retdata,
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

Var *Interpreter::eval(const ModuleLoc *loc, StringRef code, bool isExpr)
{
	Module *mod  = parser.createModule("<eval>", String(code), false);
	Var *res     = nullptr;
	int exitcode = 1;

	if(!mod || !mod->tokenize() || !mod->parseTokens(isExpr)) goto done;
	if(!mod->executeDefaultPasses()) {
		fail(loc, "Failed to apply default parser passes on module: ", mod->getPath());
		goto done;
	}
	if(!mod->genCode()) goto done;

	addModule(loc, mod, getCurrModule()->getVars());
	pushModule(mod->getPath());
	exitcode = execute(false, false);
	popModule();
	removeModule(mod->getPath());
	if(exitcode) goto done;
	if(!execstack.empty()) res = execstack.pop(false);
	else res = getNil();
done:
	if(mod) parser.removeModule(mod->getPath());
	return res;
}

void Interpreter::initTypeNames()
{
	registerType<VarAll>(nullptr, "All");

	registerType<VarNil>(nullptr, "Nil");
	registerType<VarBool>(nullptr, "Bool");
	registerType<VarInt>(nullptr, "Int");
	registerType<VarFlt>(nullptr, "Flt");
	registerType<VarStr>(nullptr, "Str");
	registerType<VarVec>(nullptr, "Vec");
	registerType<VarMap>(nullptr, "Map");
	registerType<VarFn>(nullptr, "Func");
	registerType<VarModule>(nullptr, "Module");
	registerType<VarTypeID>(nullptr, "TypeID");
	registerType<VarStructDef>(nullptr, "StructDef");
	registerType<VarStruct>(nullptr, "Struct");
	registerType<VarFile>(nullptr, "File");
	registerType<VarBytebuffer>(nullptr, "Bytebuffer");
	registerType<VarIntIterator>(nullptr, "IntIterator");
	registerType<VarVecIterator>(nullptr, "VecIterator");
	registerType<VarMapIterator>(nullptr, "MapIterator");
	registerType<VarFileIterator>(nullptr, "FileIterator");

	globals.add("AllTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarAll>()), false);

	globals.add("NilTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarNil>()), false);
	globals.add("BoolTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarBool>()), false);
	globals.add("IntTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarInt>()), false);
	globals.add("FltTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarFlt>()), false);
	globals.add("StrTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarStr>()), false);
	globals.add("VecTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarVec>()), false);
	globals.add("MapTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarMap>()), false);
	globals.add("FuncTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarFn>()), false);
	globals.add("ModuleTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarModule>()), false);
	globals.add("TypeIDTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarTypeID>()), false);
	globals.add("StructDefTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarStructDef>()),
		    false);
	globals.add("StructTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarStruct>()), false);
	globals.add("FileTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarFile>()), false);
	globals.add("BytebufferTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarBytebuffer>()),
		    false);
	globals.add("IntIteratorTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarIntIterator>()),
		    false);
	globals.add("VecIteratorTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarVecIterator>()),
		    false);
	globals.add("MapIteratorTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarMapIterator>()),
		    false);
	globals.add("FileIteratorTy", makeVarWithRef<VarTypeID>(nullptr, typeID<VarFileIterator>()),
		    false);
}

#if defined(FER_OS_WINDOWS)
bool addDLLDirectory(StringRef dir)
{
	if(dllDirectories.find(dir) != dllDirectories.end()) return true;
	DLL_DIRECTORY_COOKIE dlldir = AddDllDirectory(toWString(dir).c_str());
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