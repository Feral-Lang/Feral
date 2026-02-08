#include <chrono>
#include <cstdlib>
#include <cstring>
#include <limits.h>
#include <thread>

#include "Env.hpp"
#include "FS.hpp"
#include "VM/Interpreter.hpp"

#if defined(CORE_OS_WINDOWS)
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

#if !defined(CORE_OS_WINDOWS)
// only used for chmod
int execInternal(const String &file);
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(sleepCustom, 1, false,
           "  fn(duration) -> Nil\n"
           "Suspends the current thread for `duration` milliseconds.")
{
    if(!args[1]->is<VarInt>()) {
        vm.fail(loc, "expected integer argument for sleep time, found: ", vm.getTypeName(args[1]));
        return nullptr;
    }
    size_t dur = as<VarInt>(args[1])->getVal();
    std::this_thread::sleep_for(std::chrono::milliseconds(dur));
    return vm.getNil();
}

FERAL_FUNC(getEnv, 1, false,
           "  fn(variable) -> Str | Nil\n"
           "Returns the value of the environment `variable` as string, or `nil` if not found.")
{
    if(!args[1]->is<VarStr>()) {
        vm.fail(loc,
                "expected string argument for env variable name, found: ", vm.getTypeName(args[1]));
        return nullptr;
    }
    const String &var = as<VarStr>(args[1])->getVal();
    String res        = env::get(var.c_str());
    if(res.empty()) return vm.getNil();
    return vm.makeVar<VarStr>(loc, std::move(res));
}

FERAL_FUNC(setEnv, 3, false,
           "  fn(variable, value, overwrite = false) -> Nil\n"
           "Sets the `value` of the environment `variable` as string.\n"
           "If `overwrite` is `true`, any existing environment `variable` is overwritten.")
{
    if(!args[1]->is<VarStr>()) {
        vm.fail(loc,
                "expected string argument"
                " for env variable name, found: ",
                vm.getTypeName(args[1]));
        return nullptr;
    }
    if(!args[2]->is<VarStr>()) {
        vm.fail(loc,
                "expected string argument"
                " for env variable value, found: ",
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
    bool overwrite    = as<VarBool>(args[3])->getVal();

    return vm.makeVar<VarInt>(loc, env::set(var.c_str(), val.c_str(), overwrite));
}

FERAL_FUNC(
    execCustom, 1, true,
    "  fn(command...) -> Int\n"
    "Runs `command` on the shell and returns the exit code.\n"
    "`command` can be either a vector or a variadic of the command and its arguments/parameters.")
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
            vm.fail(loc,
                    "expected string argument for command, found: ", vm.getTypeName(argsToUse[i]));
            return nullptr;
        }
    }

    String cmd;
    for(size_t i = startFrom; i < argsToUse.size(); ++i) {
        if(i > startFrom) cmd += " ";
        StringRef arg      = as<VarStr>(argsToUse[i])->getVal();
        bool isArgOperator = !arg.empty() && arg[0] == '^';
        if(isArgOperator) arg = arg.substr(1);
        if(!isArgOperator) cmd += "\"";
        cmd += arg;
        if(!isArgOperator) cmd += "\"";
    }
#if defined(CORE_OS_WINDOWS)
    // Apparently, popen on Windows eradicates the outermost quotes.
    // If this is not present, any argument with space (including program name), will not be
    // read as a single string.
    cmd = "\"" + cmd + "\"";
#endif

    Var *outVar    = nullptr;
    auto outVarLoc = assnArgs.find("out");
    if(outVarLoc != assnArgs.end()) outVar = outVarLoc->second.val;

    Var *envVar    = nullptr;
    auto envVarLoc = assnArgs.find("env");
    if(envVarLoc != assnArgs.end()) envVar = envVarLoc->second.val;

    if(outVar && !(outVar->is<VarStr>() || outVar->is<VarVec>() || outVar->is<VarNil>())) {
        vm.fail(loc, "expected out variable to be a string/vector/nil, or not used, found: ",
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
            Var *v                  = nullptr;
            Array<Var *, 1> tmp{item.second};
            if(!vm.callVarAndExpect<VarStr>(loc, "str", v, tmp, {})) {
                vm.fail(loc, "Failed to call str() on the value of env map's key: ", item.first);
                return nullptr;
            }
            newEnv[item.first] = as<VarStr>(v)->getVal();
        }
        for(auto &item : newEnv) { env::set(item.first.c_str(), item.second.c_str(), true); }
    }

    FILE *pipe = popen(cmd.c_str(), "r");
    if(!pipe) return vm.makeVar<VarInt>(loc, 1);
    char *csline = NULL;
    size_t len   = 0;
    ssize_t nread;

    if(!outVar || outVar->is<VarNil>()) {
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
        for(auto &item : existingEnv) { env::set(item.first.c_str(), item.second.c_str(), true); }
    }

#if !defined(CORE_OS_WINDOWS)
    res = WEXITSTATUS(res);
#endif
    return vm.makeVar<VarInt>(loc, res);
}

FERAL_FUNC(systemCustom, 1, false,
           "  fn(command) -> Int\n"
           "Runs the `command` using C's `system()` function and returns the exit code.\n"
           "Here, the `command` is a single string containing the command and all parameters.")
{
    if(!args[1]->is<VarStr>()) {
        vm.fail(loc, "expected string argument for command, found: ", vm.getTypeName(args[1]));
        return nullptr;
    }
    const String &cmd = as<VarStr>(args[1])->getVal();

    int res = std::system(cmd.c_str());
#if !defined(CORE_OS_WINDOWS)
    res = WEXITSTATUS(res);
#endif
    return vm.makeVar<VarInt>(loc, res);
}

FERAL_FUNC(osStrErr, 0, false,
           "  fn(errCode) -> Str\n"
           "Returns the string equivalent for the `errCode`.")
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

FERAL_FUNC(osGetCWD, 0, false,
           "  fn() -> Str\n"
           "Returns the current working directory of the program.")
{
    VarStr *res = vm.makeVar<VarStr>(loc, fs::getCWD());
    if(res->getVal().empty()) {
        vm.fail(loc, "getCWD() failed - internal error");
        vm.decVarRef(res);
        return nullptr;
    }
    return res;
}

FERAL_FUNC(osSetCWD, 1, false,
           "  fn(dir) -> Nil\n"
           "Sets the current working directory to `dir` of the program.")
{
    if(!args[1]->is<VarStr>()) {
        vm.fail(loc, "expected string argument for destination directory, found: ",
                vm.getTypeName(args[1]));
        return nullptr;
    }
    fs::setCWD(as<VarStr>(args[1])->getVal());
    return vm.getNil();
}

#if !defined(CORE_OS_WINDOWS)
FERAL_FUNC(
    osChmod, 3, false,
    "  fn(dest, mode = '0755', recursive = true) -> Int\n"
    "Changes permissions of path `dest` as `mode` string.\n"
    "If `recursive` is `true` and `dest` is a directory, update the permissions recursively.")
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
    bool recurse       = as<VarBool>(args[3])->getVal();
    String cmd         = "chmod ";
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

    mod->addNativeFn(vm, "sleep", sleepCustom);

    mod->addNativeFn(vm, "getEnv", getEnv);
    mod->addNativeFn(vm, "setEnvNative", setEnv);

    mod->addNativeFn(vm, "exec", execCustom);
    mod->addNativeFn(vm, "system", systemCustom);
    mod->addNativeFn(vm, "strErr", osStrErr);

    mod->addNativeFn(vm, "getCWD", osGetCWD);
    mod->addNativeFn(vm, "setCWD", osSetCWD);

#if !defined(CORE_OS_WINDOWS)
    mod->addNativeFn(vm, "chmodNative", osChmod);
#endif

    return true;
}

#if !defined(CORE_OS_WINDOWS)
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

#if defined(CORE_OS_WINDOWS)
    return res;
#else
    return WEXITSTATUS(res);
#endif
}
#endif

} // namespace fer