#include <sys/wait.h>

#include "std/MultiProcType.hpp"

int execCommand(const String &cmd);

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *getConcurrency(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		    const Map<String, AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, Thread::hardware_concurrency());
}

Var *mprocNew(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for multiproc execution, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	PackagedTask<int(String)> task(execCommand);
	SharedFuture<int> *fut = new SharedFuture<int>(task.get_future());
	return vm.makeVar<VarMultiProc>(
	loc, new Thread(std::move(task), as<VarStr>(args[1])->get()), fut);
}

Var *mprocGetId(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		const Map<String, AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, as<VarMultiProc>(args[0])->getId());
}

Var *mprocIsDone(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		 const Map<String, AssnArgData> &assn_args)
{
	SharedFuture<int> *&fut = as<VarMultiProc>(args[0])->getFuture();
	if(!fut->valid()) return vm.getFalse();
	return fut->wait_for(std::chrono::seconds(0)) == std::future_status::ready ? vm.getTrue()
										   : vm.getFalse();
}

Var *mprocGetRes(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		 const Map<String, AssnArgData> &assn_args)
{
	SharedFuture<int> *&fut = as<VarMultiProc>(args[0])->getFuture();
	if(!fut->valid() || fut->wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		return vm.getNil();
	return vm.makeVar<VarInt>(loc, fut->get());
}

INIT_MODULE(MultiProc)
{
	VarModule *mod = vm.getCurrModule();

	mod->addNativeFn("getConcurrency", getConcurrency);
	mod->addNativeFn("new", mprocNew, 1);

	vm.addNativeTypeFn<VarMultiProc>(loc, "getId", mprocGetId, 0);
	vm.addNativeTypeFn<VarMultiProc>(loc, "isDone", mprocIsDone, 0);
	vm.addNativeTypeFn<VarMultiProc>(loc, "getResult", mprocGetRes, 0);
	return true;
}

// Required because popen() and pclose() are seemingly not threadsafe
static Mutex pipe_mtx;

int execCommand(const String &cmd)
{
	FILE *pipe = NULL;
	{
		LockGuard<Mutex> lock(pipe_mtx);
		pipe = popen(cmd.c_str(), "r");
	}
	if(!pipe) return 1;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	while((nread = getline(&line, &len, pipe)) != -1) {
		fprintf(stdout, "%s", line);
	}
	free(line);
	int res = 0;
	{
		LockGuard<Mutex> lock(pipe_mtx);
		res = pclose(pipe);
	}
	return WEXITSTATUS(res);
}