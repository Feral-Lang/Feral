#include "VM/InterpreterState.hpp"

#include "Env.hpp"
#include "Error.hpp"
#include "FS.hpp"
#include "Utils.hpp"
#include "VM/CoreFuncs.hpp"

#if defined(FER_OS_WINDOWS)
#include <Windows.h> // for libloaderapi.h, which contains AddDllDirectory() and RemoveDllDirectory()
#endif

namespace fer
{

Var *loadModule(InterpreterThread &vm, ModuleLoc loc, Span<Var *> args,
		const StringMap<AssnArgData> &assn_args);

#if defined(FER_OS_WINDOWS)
static StringMap<DLL_DIRECTORY_COOKIE> dllDirectories;
bool addDLLDirectory(StringRef dir);
void remDLLDirectories();
#endif

InterpreterState::InterpreterState(ArgParser &argparser, ParseSourceFn parseSourceFn)
	: argparser(argparser), parseSourceFn(parseSourceFn), mem("VM::Main"), globals(*this),
	  moduleDirs(makeVarWithRef<VarVec>(ModuleLoc(), 2, false)),
	  moduleFinders(makeVarWithRef<VarVec>(ModuleLoc(), 2, false)), prelude("prelude/prelude"),
	  binaryPath(env::getProcPath()), tru(makeVarWithRef<VarBool>(ModuleLoc(), true)),
	  fals(makeVarWithRef<VarBool>(ModuleLoc(), false)),
	  nil(makeVarWithRef<VarNil>(ModuleLoc())), recurseMax(DEFAULT_MAX_RECURSE_COUNT)
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

	VarStr *moduleLoc = makeVarWithRef<VarStr>(ModuleLoc(), INSTALL_PATH);
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

#if defined(FER_OS_WINDOWS)
	for(auto &modDir : moduleDirs->getVal()) {
		addDLLDirectory(as<VarStr>(modDir)->getVal());
	}
#endif
}

InterpreterState::~InterpreterState()
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

void InterpreterState::tryAddModulePathsFromDir(String dir)
{
	// Paths which have already been searched in for the .modulePaths file
	static Set<String> searchedPaths;
	if(searchedPaths.contains(dir)) return;
	searchedPaths.insert(dir);
	String path = dir + "/.modulePaths";
	return tryAddModulePathsFromFile(path.c_str());
}
void InterpreterState::tryAddModulePathsFromFile(const char *file)
{
	if(!fs::exists(file)) return;
	String modulePaths;
	if(!fs::read(file, modulePaths, true)) return;
	for(auto &_path : utils::stringDelim(modulePaths, "\n")) {
		if(_path.empty()) continue;
		VarStr *moduleLoc = makeVarWithRef<VarStr>(ModuleLoc(), _path);
		moduleDirs->insert(moduleDirs->begin(), moduleLoc);
	}
}

bool InterpreterState::findImportModuleIn(VarVec *dirs, String &name, StringRef srcDir)
{
	return findFileIn(dirs, name, getFeralImportExtension(), srcDir);
}
bool InterpreterState::findNativeModuleIn(VarVec *dirs, String &name, StringRef srcDir)
{
	name.insert(name.find_last_of('/') + 1, "libferal");
	return findFileIn(dirs, name, getNativeModuleExtension(), srcDir);
}
bool InterpreterState::findFileIn(VarVec *dirs, String &name, StringRef ext, StringRef srcDir)
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

void InterpreterState::addGlobal(StringRef name, Var *val, bool iref)
{
	if(globals.exists(name)) return;
	globals.add(name, val, iref);
}
Var *InterpreterState::getGlobal(StringRef name) { return globals.get(name); }

void InterpreterState::addNativeFn(ModuleLoc loc, StringRef name, NativeFn fn, size_t args,
				   bool is_va)
{
	addGlobal(name, genNativeFn(loc, name, fn, args, is_va), false);
}
VarFn *InterpreterState::genNativeFn(ModuleLoc loc, StringRef name, NativeFn fn, size_t args,
				     bool is_va)
{
	VarFn *f =
	makeVarWithRef<VarFn>(loc, -1, "", is_va ? "." : "", args, 0, FnBody{.native = fn}, true);
	for(size_t i = 0; i < args; ++i) f->pushParam("");
	return f;
}

void InterpreterState::addTypeFn(size_t _typeid, StringRef name, Var *fn, bool iref)
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
Var *InterpreterState::getTypeFn(Var *var, StringRef name)
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

void InterpreterState::setTypeName(size_t _typeid, StringRef name) { typenames[_typeid] = name; }
StringRef InterpreterState::getTypeName(size_t _typeid)
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

Var *InterpreterState::getConst(ModuleLoc loc, const Instruction::Data &d, DataType dataty)
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

bool InterpreterState::hasModule(StringRef path)
{
	for(auto &it : modules) {
		if(it.second->getPath() == path) return true;
	}
	return false;
}
VarModule *InterpreterState::getModule(StringRef path)
{
	for(auto &it : modules) {
		if(it.second->getPath() == path) return it.second;
	}
	return nullptr;
}

void InterpreterState::initTypeNames()
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