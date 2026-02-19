#include "Thread.hpp"

#include <functional>
#include <sstream>

#include "VM/VM.hpp"

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
///////////////////////////////////////////// VarThread //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarThread::VarThread(ModuleLoc loc, StringRef name, Var *_callable, Vector<Var *> &&_args,
                     VarMap *_assnArgs)
    : Var(loc, 0), name(name), callable(_callable), res(nullptr), thread(nullptr),
      args(std::move(_args)), assnArgs(_assnArgs)
{}
VarThread::~VarThread() {}

void VarThread::onCreate(VirtualMachine &vm)
{
    PackagedTask<Var *()> task(
        std::bind(&VirtualMachine::runCallable, &vm, getLoc(), name, callable, args, assnArgs));
    res    = new SharedFuture<Var *>(task.get_future());
    thread = new JThread(std::move(task));
}
void VarThread::onDestroy(VirtualMachine &vm)
{
    if(res) {
        res->wait();
        delete res;
        delete thread; // no need to join as we are using JThread
    }
    vm.decVarRef(assnArgs);
    for(auto &a : args) {
        if(a) vm.decVarRef(a);
    }
    vm.decVarRef(callable);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Common //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(getConcurrency, 0, false,
           "  fn() -> Int\n"
           "Returns the number of hardware thread contexts - number of max threads on the CPU.")
{
    return vm.makeVar<VarInt>(loc, Thread::hardware_concurrency());
}

FERAL_FUNC(getCurrentThreadId, 0, false,
           "  fn() -> Int\n"
           "Returns the ID of the current thread.")
{
    return vm.makeVar<VarInt>(loc, ThreadIdToNum(std::this_thread::get_id()));
}

FERAL_FUNC(getCurrentThreadName, 0, false,
           "  fn() -> Str\n"
           "Returns the name of the current thread.")
{
    return vm.makeVar<VarStr>(loc, vm.getName());
}

FERAL_FUNC(yieldCurrentThread, 0, false,
           "  fn() -> Nil\n"
           "Yields the current thread, freeing CPU to work on other threads on the system.")
{
    std::this_thread::yield();
    return vm.getNil();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Thread //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(threadNew, 1, true,
           "  fn(callable, args...) -> Thread\n"
           "Creates and returns a Thread instance which runs the `callable` with all of `args`.\n"
           "Accepts optional argument:\n"
           "  `name = '<threadName>'` to set the thread name, defaults to `FeralThread`.")
{
    if(!args[1]->isCallable()) {
        vm.fail(loc, "expected callable argument for multithreaded execution, found: ",
                vm.getTypeName(args[1]));
        return nullptr;
    }
    Var *namev     = assnArgs->getAttr("name");
    StringRef name = "FeralThread";
    if(namev) {
        EXPECT(VarStr, namev, "name for new thread");
        name = as<VarStr>(namev)->getVal();
    }
    VarMap *newAssnArgs = vm.copyVar(loc, assnArgs);
    if(!newAssnArgs) return nullptr;
    Var *callable = vm.incVarRef(args[1]);
    Vector<Var *> newArgs;
    Var *selfVar = newAssnArgs->getAttr("selfvar");
    newArgs.reserve(args.size() - 1); // - 2 for self + callable; + 1 for self
    newArgs.push_back(selfVar);
    if(selfVar) {
        bool found = false;
        newAssnArgs->remAttr(vm, "selfvar", found, true);
    }
    newArgs.insert(newArgs.end(), args.begin() + 2, args.end());
    for(auto &a : newArgs) vm.incVarRef(a);
    return vm.makeVar<VarThread>(loc, name, callable, std::move(newArgs), newAssnArgs);
}

FERAL_FUNC(threadGetId, 0, false,
           "  var.fn() -> Int\n"
           "Returns the ID of the Thread `var`.")
{
    return vm.makeVar<VarInt>(loc, ThreadIdToNum(as<VarThread>(args[0])->getThreadId()));
}

FERAL_FUNC(threadGetName, 0, false,
           "  var.fn() -> Str\n"
           "Returns the name of the Thread `var`.")
{
    return vm.makeVar<VarStr>(loc, as<VarThread>(args[0])->getName());
}

FERAL_FUNC(threadIsDone, 0, false,
           "  var.fn() -> Bool\n"
           "Returns `true` if the Thread `var` is finished with its task.")
{
    SharedFuture<Var *> *&fut = as<VarThread>(args[0])->getFuture();
    if(!fut->valid()) return vm.getFalse();
    return fut->wait_for(std::chrono::seconds(0)) == std::future_status::ready ? vm.getTrue()
                                                                               : vm.getFalse();
}

FERAL_FUNC(threadJoin, 0, false,
           "  var.fn() -> Var | Nil\n"
           "Joins the Thread `var` to the current thread, making the execution of the current "
           "thread wait for `var` to end.\n"
           "Returns the return value of `var`'s function, or nil if `var` wasn't valid.")
{
    SharedFuture<Var *> *&fut = as<VarThread>(args[0])->getFuture();
    if(!fut->valid()) return vm.getNil();
    fut->wait();
    as<VarThread>(args[0])->getThread()->join();
    return fut->get();
}

FERAL_FUNC(threadGetRes, 0, false,
           "  var.fn() -> Var | Nil\n"
           "Returns the return value of `var`'s function, or nil if `var` wasn't valid.")
{
    SharedFuture<Var *> *&fut = as<VarThread>(args[0])->getFuture();
    if(!fut->valid() || fut->wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        return vm.getNil();
    return fut->get();
}

INIT_DLL(Thread)
{
    vm.addLocalType<VarThread>(
        loc, "Thread", "Thread type - building block for multithreading / concurrent programming.");

    vm.addLocal(loc, "getConcurrency", getConcurrency);
    vm.addLocal(loc, "getCurrentId", getCurrentThreadId);
    vm.addLocal(loc, "getCurrentName", getCurrentThreadName);
    vm.addLocal(loc, "yield", yieldCurrentThread);

    vm.addLocal(loc, "new", threadNew);

    vm.addTypeFn<VarThread>(loc, "getId", threadGetId);
    vm.addTypeFn<VarThread>(loc, "getName", threadGetName);
    vm.addTypeFn<VarThread>(loc, "isDone", threadIsDone);
    vm.addTypeFn<VarThread>(loc, "join", threadJoin);
    vm.addTypeFn<VarThread>(loc, "getResult", threadGetRes);
    return true;
}

} // namespace fer