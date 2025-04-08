#include "MultiProc.hpp"

#include <functional>
#include <sstream>

#include "VM/Interpreter.hpp"

namespace fer
{

size_t ThreadIdToNum(Thread::id id)
{
	Map<Thread::id, size_t> threadIds;
	auto loc = threadIds.find(id);
	if(loc != threadIds.end()) return loc->second;
	std::stringstream ss;
	ss << id;
	size_t res = std::stoull(ss.str());
	threadIds.insert({id, res});
	return res;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarAtomicBool /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarAtomicBool::VarAtomicBool(ModuleLoc loc, bool _val) : Var(loc, false, false), val(_val) {}
Var *VarAtomicBool::onCopy(MemoryManager &mem, ModuleLoc loc)
{
	return makeVarWithRef<VarAtomicBool>(mem, loc, val);
}
void VarAtomicBool::onSet(MemoryManager &mem, Var *from)
{
	val = as<VarAtomicBool>(from)->getVal();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarAtomicInt //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarAtomicInt::VarAtomicInt(ModuleLoc loc, int64_t _val) : Var(loc, false, false), val(_val) {}
VarAtomicInt::VarAtomicInt(ModuleLoc loc, const char *_val)
	: Var(loc, false, false), val(std::stoll(_val))
{}
Var *VarAtomicInt::onCopy(MemoryManager &mem, ModuleLoc loc)
{
	return makeVarWithRef<VarAtomicInt>(mem, loc, val);
}
void VarAtomicInt::onSet(MemoryManager &mem, Var *from) { val = as<VarAtomicInt>(from)->getVal(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// VarMutex //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarMutex::VarMutex(ModuleLoc loc) : Var(loc, false, false) {}
VarMutex::~VarMutex() {}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// VarLockGuard ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarLockGuard::VarLockGuard(ModuleLoc loc, VarMutex *mtx) : Var(loc, false, false), mtx(mtx) {}
VarLockGuard::~VarLockGuard() {}

void VarLockGuard::onCreate(MemoryManager &mem) { incVarRef(mtx); }
void VarLockGuard::onDestroy(MemoryManager &mem) { decVarRef(mem, mtx); }

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// VarThread //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarThread::VarThread(ModuleLoc loc, StringRef name, VirtualMachine &_vm, Var *_callable,
		     Span<Var *> _args, const StringMap<AssnArgData> &_assn_args)
	: Var(loc, false, false), name(name), vm(_vm.getInterpreter().createVM()), res(nullptr),
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

	for(auto &a : args) {
		if(a) vm->incVarRef(a);
	}
	for(auto &aa : assn_args) vm->incVarRef(aa.second.val);
	PackagedTask<Var *()> task(std::bind(&VirtualMachine::callVarAndReturn, vm, getLoc(), name,
					     callable, args, assn_args));
	res    = new SharedFuture<Var *>(task.get_future());
	thread = new Thread(std::move(task));
}
VarThread::~VarThread()
{
	if(res) {
		res->wait();
		thread->join();
		delete res;
		delete thread;
	}
	for(auto &aa : assn_args) vm->decVarRef(aa.second.val);
	for(auto &a : args) {
		if(a) vm->decVarRef(a);
	}
	vm->decVarRef(callable);
	vm->getInterpreter().destroyVM(vm);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Common //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *getConcurrency(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
		    const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, Thread::hardware_concurrency());
}

Var *getCurrentThreadId(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
			const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, ThreadIdToNum(std::this_thread::get_id()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// AtomicBool /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *atomicBoolNew(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
		   const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarBool>()) {
		vm.fail(loc, "expected bool argument for creating an atomic bool, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	return vm.makeVar<VarAtomicBool>(loc, as<VarBool>(args[1])->getVal());
}

Var *atomicBoolSet(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
		   const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarBool>()) {
		vm.fail(loc, "expected int argument for setting an atomic int, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	as<VarAtomicBool>(args[0])->setVal(as<VarBool>(args[1])->getVal());
	return vm.getNil();
}

Var *atomicBoolGet(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
		   const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarBool>(loc, as<VarAtomicBool>(args[0])->getVal());
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// AtomicInt /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *atomicIntNew(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc, "expected int argument for creating an atomic int, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	return vm.makeVar<VarAtomicInt>(loc, as<VarInt>(args[1])->getVal());
}

Var *atomicIntSet(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc, "expected int argument for setting an atomic int, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	as<VarAtomicInt>(args[0])->setVal(as<VarInt>(args[1])->getVal());
	return vm.getNil();
}

Var *atomicIntGet(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, as<VarAtomicInt>(args[0])->getVal());
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Mutex ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *mutexNew(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarMutex>(loc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// LockGuard /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *lockGuardNew(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarMutex>()) {
		vm.fail(loc, "expected mutex argument for creating a lockguard, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	return vm.makeVar<VarLockGuard>(loc, as<VarMutex>(args[1]));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Thread //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *threadNew(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	       const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->isCallable()) {
		vm.fail(loc, "expected callable argument for multithreaded execution, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	auto nameLoc   = assn_args.find("name");
	StringRef name = "FeralThread";
	if(nameLoc != assn_args.end()) {
		Var *nameVar = nameLoc->second.val;
		if(!nameVar->is<VarStr>()) {
			vm.fail(loc, "name for the new thread must be a string, found: ",
				vm.getTypeName(nameVar));
			return nullptr;
		}
		name = as<VarStr>(nameVar)->getVal();
	}
	Var *callable = args[1];
	Span<Var *> passArgs(args.begin() + 2, args.end());
	VarThread *t = vm.makeVar<VarThread>(loc, name, vm, callable, passArgs, assn_args);
	return t;
}

Var *threadGetId(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
		 const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, ThreadIdToNum(as<VarThread>(args[0])->getThreadId()));
}

Var *threadGetName(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
		   const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarStr>(loc, as<VarThread>(args[0])->getName());
}

Var *threadIsDone(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	SharedFuture<Var *> *&fut = as<VarThread>(args[0])->getFuture();
	if(!fut->valid()) return vm.getFalse();
	return fut->wait_for(std::chrono::seconds(0)) == std::future_status::ready ? vm.getTrue()
										   : vm.getFalse();
}

Var *threadJoin(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
		const StringMap<AssnArgData> &assn_args)
{
	SharedFuture<Var *> *&fut = as<VarThread>(args[0])->getFuture();
	if(!fut->valid()) return vm.getNil();
	fut->wait();
	as<VarThread>(args[0])->getThread()->join();
	return fut->get();
}

Var *threadGetRes(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	SharedFuture<Var *> *&fut = as<VarThread>(args[0])->getFuture();
	if(!fut->valid() || fut->wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		return vm.getNil();
	return fut->get();
}

INIT_MODULE(MultiProc)
{
	VarModule *mod = vm.getCurrModule();

	vm.registerType<VarAtomicBool>(loc, "AtomicBool");
	vm.registerType<VarAtomicInt>(loc, "AtomicInt");
	vm.registerType<VarMutex>(loc, "Mutex");
	vm.registerType<VarLockGuard>(loc, "LockGuard");
	vm.registerType<VarThread>(loc, "Thread");

	mod->addNativeFn(vm, "getConcurrency", getConcurrency);
	mod->addNativeFn(vm, "getCurrentThreadId", getCurrentThreadId);

	mod->addNativeFn(vm, "newAtomicBool", atomicBoolNew, 1);
	mod->addNativeFn(vm, "newAtomicInt", atomicIntNew, 1);
	mod->addNativeFn(vm, "newMutex", mutexNew);
	mod->addNativeFn(vm, "newLockGuard", lockGuardNew, 1);
	mod->addNativeFn(vm, "newThread", threadNew, 1, true);

	vm.addNativeTypeFn<VarAtomicBool>(loc, "set", atomicBoolSet, 1);
	vm.addNativeTypeFn<VarAtomicBool>(loc, "get", atomicBoolGet, 0);
	vm.addNativeTypeFn<VarAtomicInt>(loc, "set", atomicIntSet, 1);
	vm.addNativeTypeFn<VarAtomicInt>(loc, "get", atomicIntGet, 0);
	vm.addNativeTypeFn<VarThread>(loc, "getId", threadGetId, 0);
	vm.addNativeTypeFn<VarThread>(loc, "getName", threadGetName, 0);
	vm.addNativeTypeFn<VarThread>(loc, "isDone", threadIsDone, 0);
	vm.addNativeTypeFn<VarThread>(loc, "join", threadJoin, 0);
	vm.addNativeTypeFn<VarThread>(loc, "getResult", threadGetRes, 0);
	return true;
}

} // namespace fer