#include <chrono>
#include <cstdlib>
#include <cstring>
#include <limits.h>
#include <sys/errno.h> // errno
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "Env.hpp"
#include "FS.hpp"
#include "Utils.hpp"
#include "VM/Interpreter.hpp"

using namespace fer;

int execInternal(const String &file);
String dirPart(const String &full_loc);

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *sleepCustom(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		 const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(
		loc, "expected integer argument for sleep time, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	size_t dur = mpz_get_ui(as<VarInt>(args[1])->get());
	std::this_thread::sleep_for(std::chrono::milliseconds(dur));
	return vm.getNil();
}

Var *getEnv(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for env variable name, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	const String &var = as<VarStr>(args[1])->get();
	VarStr *res	  = vm.makeVar<VarStr>(loc, env::get(var.c_str()));
	return res;
}

Var *setEnv(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const Map<String, AssnArgData> &assn_args)
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
		const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected string argument for command, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	const String &cmd = as<VarStr>(args[1])->get();
	VarVec *out	  = nullptr;
	if(args[2]->is<VarVec>()) out = as<VarVec>(args[2]);

	FILE *pipe = popen(cmd.c_str(), "r");
	if(!pipe) return vm.makeVar<VarInt>(loc, 1);
	char *csline = NULL;
	size_t len   = 0;
	ssize_t nread;

	if(!out) {
		while((nread = getline(&csline, &len, pipe)) != -1) {
			fprintf(stdout, "%s", csline);
		}
	} else {
		Vector<Var *> &resvec = out->get();
		String line;
		while((nread = getline(&csline, &len, pipe)) != -1) {
			line = csline;
			while(line.back() == '\n' || line.back() == '\r') line.pop_back();
			resvec.push_back(vm.makeVarWithRef<VarStr>(loc, line));
		}
	}
	if(csline) free(csline);
	int res = pclose(pipe);

	res = WEXITSTATUS(res);
	return vm.makeVar<VarInt>(loc, res);
}

Var *systemCustom(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		  const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected string argument for command, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	const String &cmd = as<VarStr>(args[1])->get();

	int res = std::system(cmd.c_str());
	res	= WEXITSTATUS(res);

	return vm.makeVar<VarInt>(loc, res);
}

Var *install(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected string argument for source, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[1]->is<VarStr>()) {
		vm.fail(
		loc, "expected string argument for destination, found: ", vm.getTypeName(args[2]));
		return nullptr;
	}
	const String &src  = as<VarStr>(args[1])->get();
	const String &dest = as<VarStr>(args[2])->get();
	if(src.empty() || dest.empty()) return vm.makeVar<VarInt>(loc, 0);

	if(execInternal("mkdir -p " + dest) != 0) return vm.makeVar<VarInt>(loc, -1);

#if __linux__ || __ANDROID__
	String cmd_str = "cp -r --remove-destination ";
#elif __APPLE__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__ || __bsdi__ || __DragonFly__
	String cmd_str = "cp -rf ";
#endif
	cmd_str += src;
	cmd_str += " ";
	cmd_str += dest;
	return vm.makeVar<VarInt>(loc, execInternal(cmd_str));
}

Var *osGetName(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<String, AssnArgData> &assn_args)
{
#if __ANDROID__
	return vm.makeVar<VarStr>(loc, "android");
#elif __linux__
	return vm.makeVar<VarStr>(loc, "linux");
#elif __APPLE__
	return vm.makeVar<VarStr>(loc, "macos");
#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__ || __bsdi__ || __DragonFly__
	return vm.makeVar<VarStr>(loc, "bsd");
#else
	return vm.makeVar<VarStr>(loc, "unknown");
#endif
}

Var *osStrErr(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc, "expected integer argument for destination directory, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	return vm.makeVar<VarStr>(loc, strerror(mpz_get_si(as<VarInt>(args[1])->get())));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// Extra Functions /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *osGetCWD(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
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
	      const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for destination directory, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	const String &dir = as<VarStr>(args[1])->get();
	return vm.makeVar<VarBool>(loc, fs::setCWD(dir.c_str()));
}

Var *osMkdir(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const Map<String, AssnArgData> &assn_args)
{
	String cmd = "mkdir -p";

	bool anyarg = false;
	for(size_t i = 1; i < args.size(); ++i) {
		if(!args[i]->is<VarStr>()) {
			vm.fail(loc, "expected string argument for directory creation, found: ",
				vm.getTypeName(args[i]));
			return nullptr;
		}
		const String &tmpdest = as<VarStr>(args[i])->get();
		if(tmpdest.empty()) continue;
		anyarg = true;
		cmd += " " + tmpdest;
	}

	if(!anyarg) return vm.makeVar<VarInt>(loc, 0);
	return vm.makeVar<VarInt>(loc, execInternal(cmd));
}

Var *osRem(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	   const Map<String, AssnArgData> &assn_args)
{
	String cmd = "rm -r";

	bool anyarg = false;
	for(size_t i = 1; i < args.size(); ++i) {
		if(!args[i]->is<VarStr>()) {
			vm.fail(loc, "expected string argument for path to delete, found: ",
				vm.getTypeName(args[i]));
			return nullptr;
		}
		const String &tmpdest = as<VarStr>(args[i])->get();
		if(tmpdest.empty()) continue;
		anyarg = true;
		cmd += " " + tmpdest;
	}

	if(!anyarg) return vm.makeVar<VarInt>(loc, 0);
	return vm.makeVar<VarInt>(loc, execInternal(cmd));
}

Var *osCopy(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const Map<String, AssnArgData> &assn_args)
{
	String cmd = "cp -r";
	// last element is the destination
	bool anyarg = false;
	for(size_t i = 1; i < args.size(); ++i) {
		if(!args[i]->is<VarStr>()) {
			vm.fail(loc, "expected string argument for copy argument, found: ",
				vm.getTypeName(args[i]));
			return nullptr;
		}
		const String &tmpdest = as<VarStr>(args[i])->get();
		if(tmpdest.empty()) continue;
		anyarg = true;
		cmd += " " + tmpdest;
	}

	if(!anyarg) return vm.makeVar<VarInt>(loc, 0);
	return vm.makeVar<VarInt>(loc, execInternal(cmd));
}

Var *osChmod(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const Map<String, AssnArgData> &assn_args)
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

Var *osMov(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	   const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for from, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for to, found: ", vm.getTypeName(args[2]));
		return nullptr;
	}
	const char *from = as<VarStr>(args[1])->get().c_str();
	const char *to	 = as<VarStr>(args[2])->get().c_str();
	return vm.makeVar<VarInt>(loc, fs::rename(from, to));
}

INIT_MODULE(OS)
{
	VarModule *mod = vm.getCurrModule();

	mod->addNativeFn("sleep", sleepCustom, 1);

	mod->addNativeFn("getEnv", getEnv, 1);
	mod->addNativeFn("setEnvNative", setEnv, 3);

	mod->addNativeFn("execNative", execCustom, 2);
	mod->addNativeFn("system", systemCustom, 1);
	mod->addNativeFn("install", install, 2);
	mod->addNativeFn("strErr", osStrErr, 1);
	mod->addNativeFn("getNameNative", osGetName);

	mod->addNativeFn("getCWD", osGetCWD);
	mod->addNativeFn("setCWD", osSetCWD, 1);

	mod->addNativeFn("mkdir", osMkdir, 1, true);
	mod->addNativeFn("rm", osRem, 1, true);

	mod->addNativeFn("cp", osCopy, 2, true);
	mod->addNativeFn("mv", osMov, 2);

	mod->addNativeFn("chmodNative", osChmod, 3);

	return true;
}

int execInternal(const String &cmd)
{
	FILE *pipe = popen(cmd.c_str(), "r");
	if(!pipe) return 1;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;

	while((nread = getline(&line, &len, pipe)) != -1)
		;
	free(line);
	int res = pclose(pipe);
	return WEXITSTATUS(res);
}

String dirPart(const String &full_loc)
{
	auto loc = full_loc.find_last_of('/');
	if(loc == String::npos) return ".";
	if(loc == 0) return "/";
	return full_loc.substr(0, loc);
}
