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

Var *sleepCustom(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		 const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(
		loc, "expected integer argument for sleep time, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	size_t dur = as<VarInt>(args[1])->get();
	std::this_thread::sleep_for(std::chrono::milliseconds(dur));
	return vm.getNil();
}

Var *getEnv(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for env variable name, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	const String &var = as<VarStr>(args[1])->get();
	return vm.makeVar<VarStr>(loc, env::get(var.c_str()));
}

Var *setEnv(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
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

	const String &var = as<VarStr>(args[1])->get();
	const String &val = as<VarStr>(args[2])->get();
	bool overwrite	  = as<VarBool>(args[3])->get();

	return vm.makeVar<VarInt>(loc, env::set(var.c_str(), val.c_str(), overwrite));
}

Var *execCustom(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected string argument for command, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	const String &cmd = as<VarStr>(args[1])->get();

	FILE *pipe = popen(cmd.c_str(), "r");
	if(!pipe) return vm.makeVar<VarInt>(loc, 1);
	char *csline = NULL;
	size_t len   = 0;
	ssize_t nread;

	if(args[2]->is<VarNil>()) {
		while((nread = getline(&csline, &len, pipe)) != -1) std::cout << csline;
	} else if(args[2]->is<VarVec>()) {
		Vector<Var *> &resvec = as<VarVec>(args[2])->get();
		String line;
		while((nread = getline(&csline, &len, pipe)) != -1) {
			line = csline;
			while(line.back() == '\n' || line.back() == '\r') line.pop_back();
			resvec.push_back(vm.makeVarWithRef<VarStr>(loc, line));
		}
	} else if(args[2]->is<VarStr>()) {
		String &resstr = as<VarStr>(args[2])->get();
		while((nread = getline(&csline, &len, pipe)) != -1) {
			resstr += csline;
			while(resstr.back() == '\n' || resstr.back() == '\r') resstr.pop_back();
		}
	}
	if(csline) free(csline);
	int res = pclose(pipe);

#if !defined(FER_OS_WINDOWS)
	res = WEXITSTATUS(res);
#endif
	return vm.makeVar<VarInt>(loc, res);
}

Var *systemCustom(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected string argument for command, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	const String &cmd = as<VarStr>(args[1])->get();

	int res = std::system(cmd.c_str());
#if !defined(FER_OS_WINDOWS)
	res = WEXITSTATUS(res);
#endif
	return vm.makeVar<VarInt>(loc, res);
}

Var *osGetName(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
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

Var *osStrErr(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc, "expected integer argument for destination directory, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	return vm.makeVar<VarStr>(loc, strerror(as<VarInt>(args[1])->get()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// Extra Functions /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *osGetCWD(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	VarStr *res = vm.makeVar<VarStr>(loc, fs::getCWD());
	if(res->get().empty()) {
		vm.fail(loc, "getCWD() failed - internal error");
		vm.unmakeVar(res);
		return nullptr;
	}
	return res;
}

Var *osSetCWD(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for destination directory, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	const String &dir = as<VarStr>(args[1])->get();
	return vm.makeVar<VarBool>(loc, fs::setCWD(dir.c_str()));
}

#if !defined(FER_OS_WINDOWS)
Var *osChmod(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
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
	const String &dest = as<VarStr>(args[1])->get();
	const String &mode = as<VarStr>(args[2])->get();
	bool recurse	   = as<VarBool>(args[3])->get();
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

	mod->addNativeFn("sleep", sleepCustom, 1);

	mod->addNativeFn("getEnv", getEnv, 1);
	mod->addNativeFn("setEnvNative", setEnv, 3);

	mod->addNativeFn("execNative", execCustom, 2);
	mod->addNativeFn("system", systemCustom, 1);
	mod->addNativeFn("strErr", osStrErr, 1);
	mod->addNativeFn("getNameNative", osGetName);

	mod->addNativeFn("getCWD", osGetCWD);
	mod->addNativeFn("setCWD", osSetCWD, 1);

#if !defined(FER_OS_WINDOWS)
	mod->addNativeFn("chmodNative", osChmod, 3);
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