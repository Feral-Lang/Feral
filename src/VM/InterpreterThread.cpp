#include "VM/InterpreterThread.hpp"

#include "Error.hpp"
#include "FS.hpp"
#include "Utils.hpp"
#include "VM/CoreFuncs.hpp"
#include "VM/DynLib.hpp"
#include "VM/Interpreter.hpp"

namespace fer
{

Var *loadModule(InterpreterThread &vm, ModuleLoc loc, Span<Var *> args,
		const StringMap<AssnArgData> &assn_args);

InterpreterThread::InterpreterThread(Interpreter &interp)
	: interp(interp), globalState(interp.getGlobalState()), failstack(*this), execstack(*this),
	  recurseCount(0), exitcode(0), recurseExceeded(false), exitcalled(false)
{}

InterpreterThread::~InterpreterThread() {}

int InterpreterThread::compileAndRun(ModuleLoc loc, const char *file)
{
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

	if(globalState.argparser.has("dry")) return 0;

	pushModule(moduleId);
	int res = execute();
	popModule();
	return res;
}

ModuleId InterpreterThread::addModule(ModuleLoc loc, StringRef path, String &&code,
				      bool virtualPath, bool exprOnly, Vars *existingVars)
{
	static ModuleId moduleIdCtr = 0;
	Bytecode bc;
	if(!globalState.parseSourceFn(*this, bc, moduleIdCtr, path, code, exprOnly)) {
		fail(loc, "failed to parse source: ", path);
		return -1;
	}
	err.setPathForId(moduleIdCtr, path);
	if(virtualPath) err.setCodeForId(moduleIdCtr, std::move(code));
	VarModule *mod =
	makeVarWithRef<VarModule>(loc, path, std::move(bc), moduleIdCtr, existingVars);
	LockGuard<Mutex> globalGuard(globalState.globalMutex);
	globalState.modules.insert_or_assign(moduleIdCtr, mod);
	return moduleIdCtr++;
}
void InterpreterThread::removeModule(ModuleId moduleId)
{
	auto loc = globalState.modules.find(moduleId);
	if(loc == globalState.modules.end()) return;
	LockGuard<Mutex> globalGuard(globalState.globalMutex);
	decVarRef(loc->second);
	globalState.modules.erase(loc);
}
void InterpreterThread::pushModule(ModuleId moduleId)
{
	auto mloc = globalState.modules.find(moduleId);
	modulestack.push_back(mloc->second);
}
void InterpreterThread::popModule() { modulestack.pop_back(); }

bool InterpreterThread::findImportModuleIn(VarVec *dirs, String &name)
{
	return findFileIn(dirs, name, getFeralImportExtension());
}
bool InterpreterThread::findNativeModuleIn(VarVec *dirs, String &name)
{
	name.insert(name.find_last_of('/') + 1, "libferal");
	return findFileIn(dirs, name, getNativeModuleExtension());
}
bool InterpreterThread::findFileIn(VarVec *dirs, String &name, StringRef ext)
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

bool InterpreterThread::loadNativeModule(ModuleLoc loc, const String &modpath, StringRef moduleStr)
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
	if(deinitfn) globalState.dlldeinitfns[modpath] = deinitfn;
	return true;
}

bool InterpreterThread::callVar(ModuleLoc loc, StringRef name, Var *&retdata, Span<Var *> args,
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

bool InterpreterThread::callVar(ModuleLoc loc, StringRef name, Var *callable, Var *&retdata,
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
Var *InterpreterThread::callVarThreaded(ModuleLoc loc, String name, Var *callable, Span<Var *> args,
					const StringMap<AssnArgData> &assn_args)
{
	Var *retData = nullptr;
	bool res     = callVar(loc, name, callable, retData, args, assn_args);
	if(!res) return nullptr;
	return retData;
}

Var *InterpreterThread::eval(ModuleLoc loc, StringRef code, bool isExpr)
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

} // namespace fer