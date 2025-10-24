#include "Thread.hpp"

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
///////////////////////////////////////////// VarThread //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarThread::VarThread(ModuleLoc loc, StringRef name, Interpreter &_ip, Var *_callable,
                     Span<Var *> _args, const StringMap<AssnArgData> &_assnArgs)
    : Var(loc, false, false), name(name), res(nullptr), thread(nullptr), ip(_ip),
      callable(_callable)
{
    args.reserve(_args.size() + 1); // +1 for self/nullptr
    auto selfArgLoc = _assnArgs.find("selfVar");
    Var *selfVar    = nullptr;
    if(selfArgLoc != _assnArgs.end()) {
        selfVar = selfArgLoc->second.val;
    }
    args.push_back(selfVar);
    args.insert(args.end(), _args.begin(), _args.end());

    assnArgs.reserve(_assnArgs.size() - (selfVar != nullptr ? 1 : 0));
    for(auto &aa : _assnArgs) {
        if(aa.first == "selfVar") continue;
        assnArgs.insert(aa);
    }
}
VarThread::~VarThread() {}

void VarThread::onCreate(MemoryManager &mem)
{
    Var::incVarRef(callable);
    for(auto &a : args) {
        if(a) Var::incVarRef(a);
    }
    for(auto &aa : assnArgs) Var::incVarRef(aa.second.val);
    PackagedTask<Var *()> task(
        std::bind(&Interpreter::runCallable, &ip, getLoc(), name, callable, args, assnArgs));
    res    = new SharedFuture<Var *>(task.get_future());
    thread = new JThread(std::move(task));
}
void VarThread::onDestroy(MemoryManager &mem)
{
    if(res) {
        res->wait();
        delete res;
        delete thread; // no need to join as we are using JThread
    }
    for(auto &aa : assnArgs) Var::decVarRef(mem, aa.second.val);
    for(auto &a : args) {
        if(a) Var::decVarRef(mem, a);
    }
    Var::decVarRef(mem, callable);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Common //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *getConcurrency(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                    const StringMap<AssnArgData> &assnArgs)
{
    return vm.makeVar<VarInt>(loc, Thread::hardware_concurrency());
}

Var *getCurrentThreadId(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                        const StringMap<AssnArgData> &assnArgs)
{
    return vm.makeVar<VarInt>(loc, ThreadIdToNum(std::this_thread::get_id()));
}

Var *getCurrentThreadName(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                          const StringMap<AssnArgData> &assnArgs)
{
    return vm.makeVar<VarStr>(loc, vm.getName());
}

Var *yieldCurrentThread(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                        const StringMap<AssnArgData> &assnArgs)
{
    std::this_thread::yield();
    return vm.getNil();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Thread //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *threadNew(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
               const StringMap<AssnArgData> &assnArgs)
{
    if(!args[1]->isCallable()) {
        vm.fail(loc, "expected callable argument for multithreaded execution, found: ",
                vm.getTypeName(args[1]));
        return nullptr;
    }
    auto nameLoc   = assnArgs.find("name");
    StringRef name = "FeralThread";
    if(nameLoc != assnArgs.end()) {
        Var *nameVar = nameLoc->second.val;
        if(!nameVar->is<VarStr>()) {
            vm.fail(loc,
                    "name for the new thread must be a string, found: ", vm.getTypeName(nameVar));
            return nullptr;
        }
        name = as<VarStr>(nameVar)->getVal();
    }
    Var *callable = args[1];
    Span<Var *> passArgs(args.begin() + 2, args.end());
    VarThread *t =
        vm.makeVar<VarThread>(loc, name, vm.getInterpreter(), callable, passArgs, assnArgs);
    return t;
}

Var *threadGetId(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                 const StringMap<AssnArgData> &assnArgs)
{
    return vm.makeVar<VarInt>(loc, ThreadIdToNum(as<VarThread>(args[0])->getThreadId()));
}

Var *threadGetName(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                   const StringMap<AssnArgData> &assnArgs)
{
    return vm.makeVar<VarStr>(loc, as<VarThread>(args[0])->getName());
}

Var *threadIsDone(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                  const StringMap<AssnArgData> &assnArgs)
{
    SharedFuture<Var *> *&fut = as<VarThread>(args[0])->getFuture();
    if(!fut->valid()) return vm.getFalse();
    return fut->wait_for(std::chrono::seconds(0)) == std::future_status::ready ? vm.getTrue()
                                                                               : vm.getFalse();
}

Var *threadJoin(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                const StringMap<AssnArgData> &assnArgs)
{
    SharedFuture<Var *> *&fut = as<VarThread>(args[0])->getFuture();
    if(!fut->valid()) return vm.getNil();
    fut->wait();
    as<VarThread>(args[0])->getThread()->join();
    return fut->get();
}

Var *threadGetRes(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                  const StringMap<AssnArgData> &assnArgs)
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
    mod->addNativeFn(vm, "getCurrentId", getCurrentThreadId);
    mod->addNativeFn(vm, "getCurrentName", getCurrentThreadName);
    mod->addNativeFn(vm, "yield", yieldCurrentThread);

    mod->addNativeFn(vm, "new", threadNew, 1, true);

    vm.addNativeTypeFn<VarThread>(loc, "getId", threadGetId, 0);
    vm.addNativeTypeFn<VarThread>(loc, "getName", threadGetName, 0);
    vm.addNativeTypeFn<VarThread>(loc, "isDone", threadIsDone, 0);
    vm.addNativeTypeFn<VarThread>(loc, "join", threadJoin, 0);
    vm.addNativeTypeFn<VarThread>(loc, "getResult", threadGetRes, 0);
    return true;
}

} // namespace fer