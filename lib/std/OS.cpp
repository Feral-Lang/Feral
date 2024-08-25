#include <chrono>
#include <cstdlib>
#include <cstring>
#include <limits.h>
#include <thread>

#include "Env.hpp"
#include "FS.hpp"
#include "VM/Interpreter.hpp"

#if defined(FER_OS_WINDOWS)
// Windows doesn't have peopen/pclose, but it does have an underscore version!
#define popen _popen
#define pclose _pclose
#else
#include <sys/errno.h> // errno
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fer
{

#if !defined(FER_OS_WINDOWS)
// only used for chmod
int execInternal(const String &file);
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *sleepCustom(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
		 const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(
		loc, "expected integer argument for sleep time, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	size_t dur = as<VarInt>(args[1])->getVal();
	std::this_thread::sleep_for(std::chrono::milliseconds(dur));
	return vm.getNil();
}

Var *getEnv(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
	    const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for env variable name, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	const String &var = as<VarStr>(args[1])->getVal();
	return vm.makeVar<VarStr>(loc, env::get(var.c_str()));
}

Var *setEnv(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
	    const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for env variable name, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for env variable value, found: ",
			vm.getTypeName(args[2]));
		return nullptr;
	}
	if(!args[3]->is<VarBool>()) {
		vm.fail(loc,
			"expected boolean argument for overwriting"
			" existing env variable, found: ",
			vm.getTypeName(args[3]));
		return nullptr;
	}

	const String &var = as<VarStr>(args[1])->getVal();
	const String &val = as<VarStr>(args[2])->getVal();
	bool overwrite	  = as<VarBool>(args[3])->getVal();

	return vm.makeVar<VarInt>(loc, env::set(var.c_str(), val.c_str(), overwrite));
}

Var *execCustom(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
		const StringMap<AssnArgData> &assn_args)
{
	Span<Var *> argsToUse = args;
	size_t startFrom      = 1;

	if(args[1]->is<VarVec>()) {
		auto &vec = as<VarVec>(args[1])->getVal();
		argsToUse = vec;
		startFrom = 0;
	}

	for(size_t i = startFrom; i < argsToUse.size(); ++i) {
		if(!argsToUse[i]->is<VarStr>()) {
			vm.fail(loc, "expected string argument for command, found: ",
				vm.getTypeName(argsToUse[i]));
			return nullptr;
		}
	}

	String cmd;
	for(size_t i = startFrom; i < argsToUse.size(); ++i) {
		if(i > startFrom) cmd += " ";
		StringRef arg	   = as<VarStr>(argsToUse[i])->getVal();
		bool isArgOperator = !arg.empty() && arg[0] == '^';
		if(isArgOperator) arg = arg.substr(1);
		if(!isArgOperator) cmd += "\"";
		cmd += arg;
		if(!isArgOperator) cmd += "\"";
	}
#if defined(FER_OS_WINDOWS)
	// Apparently, popen on Windows eradicates the outermost quotes.
	// If this is not present, any argument with space (including program name), will not be
	// read as a single string.
	cmd = "\"" + cmd + "\"";
#endif

	Var *outVar    = nullptr;
	auto outVarLoc = assn_args.find("out");
	if(outVarLoc != assn_args.end()) outVar = outVarLoc->second.val;

	Var *envVar    = nullptr;
	auto envVarLoc = assn_args.find("env");
	if(envVarLoc != assn_args.end()) envVar = envVarLoc->second.val;

	if(outVar && !(outVar->is<VarStr>() || outVar->is<VarVec>())) {
		vm.fail(loc, "expected out variable to be a string/vector, or not used, found: ",
			vm.getTypeName(outVar));
		return nullptr;
	}

	if(envVar && !envVar->is<VarMap>()) {
		vm.fail(loc, "expected env variable to be a map, found: ", vm.getTypeName(envVar));
		return nullptr;
	}

	StringMap<String> existingEnv;
	// this is made to convert Feral's map of string,Var* without tainting env variables should
	// the conversion fail.
	StringMap<String> newEnv;
	if(envVar) {
		auto &map = as<VarMap>(envVar)->getVal();
		String val;
		for(auto &item : map) {
			val = env::get(item.first.c_str());
			// Must add keys with empty values as well to clean out the env afterwards.
			existingEnv[item.first] = val;
			Var *v			= nullptr;
			Array<Var *, 1> tmp{item.second};
			if(!vm.callVarAndExpect<VarStr>(loc, "str", v, tmp, {})) {
				vm.fail(loc, "Failed to call str() on the value of env map's key: ",
					item.first);
				return nullptr;
			}
			newEnv[item.first] = as<VarStr>(v)->getVal();
		}
		for(auto &item : newEnv) {
			env::set(item.first.c_str(), item.second.c_str(), true);
		}
	}

	FILE *pipe = popen(cmd.c_str(), "r");
	if(!pipe) return vm.makeVar<VarInt>(loc, 1);
	char *csline = NULL;
	size_t len   = 0;
	ssize_t nread;

	if(!outVar) {
		while((nread = getline(&csline, &len, pipe)) != -1) std::cout << csline;
	} else if(outVar->is<VarVec>()) {
		Vector<Var *> &resvec = as<VarVec>(outVar)->getVal();
		String line;
		while((nread = getline(&csline, &len, pipe)) != -1) {
			line = csline;
			while(!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
				line.pop_back();
			}
			resvec.push_back(vm.makeVarWithRef<VarStr>(loc, line));
		}
	} else if(outVar->is<VarStr>()) {
		String &resstr = as<VarStr>(outVar)->getVal();
		while((nread = getline(&csline, &len, pipe)) != -1) {
			resstr += csline;
			while(!resstr.empty() && (resstr.back() == '\n' || resstr.back() == '\r')) {
				resstr.pop_back();
			}
		}
	}
	if(csline) free(csline);
	int res = pclose(pipe);

	if(envVar) {
		for(auto &item : existingEnv) {
			env::set(item.first.c_str(), item.second.c_str(), true);
		}
	}

#if !defined(FER_OS_WINDOWS)
	res = WEXITSTATUS(res);
#endif
	return vm.makeVar<VarInt>(loc, res);
}

Var *systemCustom(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected string argument for command, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	const String &cmd = as<VarStr>(args[1])->getVal();

	int res = std::system(cmd.c_str());
#if !defined(FER_OS_WINDOWS)
	res = WEXITSTATUS(res);
#endif
	return vm.makeVar<VarInt>(loc, res);
}

Var *osGetName(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
	       const StringMap<AssnArgData> &assn_args)
{
#if defined(FER_OS_WINDOWS)
	return vm.makeVar<VarStr>(loc, "windows");
#elif defined(FER_OS_ANDROID)
	return vm.makeVar<VarStr>(loc, "android");
#elif defined(FER_OS_LINUX)
	return vm.makeVar<VarStr>(loc, "linux");
#elif defined(FER_OS_APPLE)
	return vm.makeVar<VarStr>(loc, "macos");
#elif defined(FER_OS_BSD)
	return vm.makeVar<VarStr>(loc, "bsd");
#else
	return vm.makeVar<VarStr>(loc, "unknown");
#endif
}

Var *osStrErr(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc, "expected integer argument for destination directory, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	return vm.makeVar<VarStr>(loc, strerror(as<VarInt>(args[1])->getVal()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// Extra Functions /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *osGetCWD(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	VarStr *res = vm.makeVar<VarStr>(loc, fs::getCWD());
	if(res->getVal().empty()) {
		vm.fail(loc, "getCWD() failed - internal error");
		vm.unmakeVar(res);
		return nullptr;
	}
	return res;
}

Var *osSetCWD(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for destination directory, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	const String &dir = as<VarStr>(args[1])->getVal();
	return vm.makeVar<VarBool>(loc, fs::setCWD(dir.c_str()));
}

#if !defined(FER_OS_WINDOWS)
Var *osChmod(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
	     const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected string argument"
			" for destination, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarStr>()) {
		vm.fail(loc,
			"expected string argument"
			" for mode, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[3]->is<VarBool>()) {
		vm.fail(loc,
			"expected boolean argument"
			" for recursive, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	const String &dest = as<VarStr>(args[1])->getVal();
	const String &mode = as<VarStr>(args[2])->getVal();
	bool recurse	   = as<VarBool>(args[3])->getVal();
	String cmd	   = "chmod ";
	if(recurse) cmd += "-R ";
	cmd += mode;
	cmd += " ";
	cmd += dest;
	return vm.makeVar<VarInt>(loc, execInternal(cmd));
}
#endif

INIT_MODULE(OS)
{
	VarModule *mod = vm.getCurrModule();

	mod->addNativeFn(vm, "sleep", sleepCustom, 1);

	mod->addNativeFn(vm, "getEnv", getEnv, 1);
	mod->addNativeFn(vm, "setEnvNative", setEnv, 3);

	mod->addNativeFn(vm, "exec", execCustom, 1, true);
	mod->addNativeFn(vm, "system", systemCustom, 1);
	mod->addNativeFn(vm, "strErr", osStrErr, 1);
	mod->addNativeFn(vm, "getNameNative", osGetName);

	mod->addNativeFn(vm, "getCWD", osGetCWD);
	mod->addNativeFn(vm, "setCWD", osSetCWD, 1);

#if !defined(FER_OS_WINDOWS)
	mod->addNativeFn(vm, "chmodNative", osChmod, 3);
#endif

	return true;
}

#if !defined(FER_OS_WINDOWS)
int execInternal(const String &cmd)
{
	FILE *pipe = popen(cmd.c_str(), "r");
	if(!pipe) return 1;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;

	while((nread = getline(&line, &len, pipe)) != -1);
	free(line);
	int res = pclose(pipe);

#if defined(FER_OS_WINDOWS)
	return res;
#else
	return WEXITSTATUS(res);
#endif
}
#endif

} // namespace fer