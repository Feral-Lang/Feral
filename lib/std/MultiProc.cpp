#include "std/MultiProc.hpp"

#include "VM/Interpreter.hpp"

#if defined(FER_OS_WINDOWS)
// Windows doesn't have peopen/pclose, but it does have an underscore version!
#define popen _popen
#define pclose _pclose
#include "FS.hpp" // for getline()
#else
#include <sys/wait.h>
#endif

namespace fer
{

static size_t threadId = 0;

VarMultiProc::VarMultiProc(const ModuleLoc *loc, Thread *thread, SharedFuture<int> *res, bool owner)
	: Var(loc, false, false), thread(thread), res(res), id(threadId++), owner(owner)
{}
VarMultiProc::VarMultiProc(const ModuleLoc *loc, Thread *thread, SharedFuture<int> *res, size_t id,
			   bool owner)
	: Var(loc, false, false), thread(thread), res(res), id(id), owner(owner)
{}
VarMultiProc::~VarMultiProc()
{
	if(owner) {
		if(thread != nullptr) {
			thread->join();
			delete thread;
		}
		if(res != nullptr) delete res;
	}
}
Var *VarMultiProc::onCopy(Interpreter &vm, const ModuleLoc *loc)
{
	return vm.makeVarWithRef<VarMultiProc>(loc, thread, res, id, false);
}
void VarMultiProc::onSet(Interpreter &vm, Var *from)
{
	VarMultiProc *t = as<VarMultiProc>(from);
	owner		= false;
	thread		= t->thread;
	res		= t->res;
	id		= t->id;
}

int execCommand(const String &cmd);

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *getConcurrency(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		    const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, Thread::hardware_concurrency());
}

Var *mprocNew(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for multiproc execution, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	PackagedTask<int(String)> task(execCommand);
	SharedFuture<int> *fut = new SharedFuture<int>(task.get_future());
	return vm.makeVar<VarMultiProc>(
	loc, new Thread(std::move(task), as<VarStr>(args[1])->getVal()), fut);
}

Var *mprocGetId(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, as<VarMultiProc>(args[0])->getId());
}

Var *mprocIsDone(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		 const StringMap<AssnArgData> &assn_args)
{
	SharedFuture<int> *&fut = as<VarMultiProc>(args[0])->getFuture();
	if(!fut->valid()) return vm.getFalse();
	return fut->wait_for(std::chrono::seconds(0)) == std::future_status::ready ? vm.getTrue()
										   : vm.getFalse();
}

Var *mprocGetRes(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		 const StringMap<AssnArgData> &assn_args)
{
	SharedFuture<int> *&fut = as<VarMultiProc>(args[0])->getFuture();
	if(!fut->valid() || fut->wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		return vm.getNil();
	return vm.makeVar<VarInt>(loc, fut->get());
}

INIT_MODULE(MultiProc)
{
	VarModule *mod = vm.getCurrModule();

	vm.registerType<VarMultiProc>(loc, "MultiProc");

	mod->addNativeFn(vm, "getConcurrency", getConcurrency);
	mod->addNativeFn(vm, "new", mprocNew, 1);

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
#if defined(FER_OS_WINDOWS)
	return res;
#else
	return WEXITSTATUS(res);
#endif
}

} // namespace fer