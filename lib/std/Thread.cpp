#include "Thread.hpp"

#include <functional>

#include "VM/Interpreter.hpp"

namespace fer
{

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// VarMutex //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarMutex::VarMutex(ModuleLoc loc) : Var(loc, false, false) {}
VarMutex::~VarMutex() {}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// VarLockGuard ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarLockGuard::VarLockGuard(ModuleLoc loc, VarMutex *mtx)
	: Var(loc, false, false), lg(mtx->getMutex())
{}
VarLockGuard::~VarLockGuard() {}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// VarThread //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarThread::VarThread(ModuleLoc loc, InterpreterThread &currVM, Var *_callable, Span<Var *> _args,
		     const StringMap<AssnArgData> &_assn_args)
	: Var(loc, false, false), vm(currVM.getInterpreter().createThread()), res(nullptr),
	  thread(nullptr), callable(_callable)
{
	vm->incVarRef(callable);

	args.reserve(_args.size() + 1); // +1 for self/nullptr
	auto selfArgLoc = _assn_args.find("selfVar");
	Var *selfVar	= nullptr;
	if(selfArgLoc != _assn_args.end()) {
		selfVar = selfArgLoc->second.val;
	}
	args.push_back(selfVar);
	args.insert(args.end(), _args.begin(), _args.end());

	assn_args.reserve(_assn_args.size() - (selfVar != nullptr ? 1 : 0));
	for(auto &aa : _assn_args) {
		if(aa.first == "selfVar") continue;
		assn_args.insert(aa);
	}

	for(auto &aa : assn_args) vm->incVarRef(aa.second.val);
	for(auto &a : args) {
		if(a) vm->incVarRef(a);
	}
	PackagedTask<Var *()> task(std::bind(&InterpreterThread::callVarThreaded, vm, getLoc(),
					     "threadNew", callable, args, assn_args));
	res    = new SharedFuture<Var *>(task.get_future());
	thread = new Thread(std::move(task));
}
VarThread::~VarThread()
{
	if(res) {
		res->wait();
		delete res;
		thread->join();
		delete thread;
	}
	for(auto &aa : assn_args) vm->decVarRef(aa.second.val);
	for(auto &a : args) {
		if(a) vm->decVarRef(a);
	}
	vm->decVarRef(callable);
	vm->getInterpreter().destroyThread(vm);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *getConcurrency(InterpreterThread &vm, ModuleLoc loc, Span<Var *> args,
		    const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, Thread::hardware_concurrency());
}

Var *threadNew(InterpreterThread &vm, ModuleLoc loc, Span<Var *> args,
	       const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->isCallable()) {
		vm.fail(loc, "expected callable argument for multithreaded execution, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	Var *callable = args[1];
	Span<Var *> passArgs(args.begin() + 2, args.end());
	VarThread *t = vm.makeVar<VarThread>(loc, vm, callable, passArgs, assn_args);
	return t;
}

Var *threadGetId(InterpreterThread &vm, ModuleLoc loc, Span<Var *> args,
		 const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, as<VarThread>(args[0])->getThreadId());
}

Var *threadIsDone(InterpreterThread &vm, ModuleLoc loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	SharedFuture<Var *> *&fut = as<VarThread>(args[0])->getFuture();
	if(!fut->valid()) return vm.getFalse();
	return fut->wait_for(std::chrono::seconds(0)) == std::future_status::ready ? vm.getTrue()
										   : vm.getFalse();
}

Var *threadJoin(InterpreterThread &vm, ModuleLoc loc, Span<Var *> args,
		const StringMap<AssnArgData> &assn_args)
{
	SharedFuture<Var *> *&fut = as<VarThread>(args[0])->getFuture();
	if(!fut->valid()) return vm.getNil();
	fut->wait();
	return fut->get();
}

Var *threadGetRes(InterpreterThread &vm, ModuleLoc loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	SharedFuture<Var *> *&fut = as<VarThread>(args[0])->getFuture();
	if(!fut->valid() || fut->wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		return vm.getNil();
	return fut->get();
}

INIT_MODULE(Thread)
{
	VarModule *mod = vm.getCurrModule();

	vm.registerType<VarThread>(loc, "Thread");

	mod->addNativeFn(vm, "getConcurrency", getConcurrency);
	mod->addNativeFn(vm, "new", threadNew, 1, true);

	vm.addNativeTypeFn<VarThread>(loc, "getId", threadGetId, 0);
	vm.addNativeTypeFn<VarThread>(loc, "isDone", threadIsDone, 0);
	vm.addNativeTypeFn<VarThread>(loc, "join", threadJoin, 0);
	vm.addNativeTypeFn<VarThread>(loc, "getResult", threadGetRes, 0);
	return true;
}

} // namespace fer