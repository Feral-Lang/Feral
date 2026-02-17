#include "VM/FailStack.hpp"

#include "VM/VM.hpp"

namespace fer
{

FailStack::FailStack(VirtualMachine &vm) : vm(vm) {}
FailStack::~FailStack()
{
    assert(size() <= 1 && "Failstack must have <= 1 items before it is destroyed");
    for(auto &s : stack) { vm.decVarRef(s); }
}

void FailStack::pushHandler(VarFn *handler, size_t popLoc, size_t recurseCount)
{
    VarFailure *failure = vm.makeVar<VarFailure>(handler->getLoc(), handler, popLoc, recurseCount);
    stack.push_back(vm.incVarRef(failure));
}

void FailStack::popHandler()
{
    vm.decVarRef(stack.back());
    stack.pop_back();
}

Var *FailStack::handle(VirtualMachine &vm, ModuleLoc loc, size_t &popLoc)
{
    if(stack.empty() || stack.back()->isHandling()) return nullptr;
    VarFailure *f = stack.back();
    vm.incVarRef(f);
    Array<Var *, 2> args{nullptr, f};
    Var *res = f->callHandler(vm, loc, args);
    popLoc   = f->getPopLoc();
    vm.decVarRef(f);
    if(!res) popHandler();
    return res;
}

void FailStack::failStr(ModuleLoc loc, size_t recurseCount, String &&msg)
{
    if(!recurseCount || stack.empty()) {
        err.fail(loc, msg);
        return;
    }
    VarFailure *f = stack.back();
    if(f->isHandling()) {
        err.fail(loc, "Encountered error while handling another: ", msg);
        return;
    }
    f->pushFrame(loc);
    if(f->hasMsg()) return;
    f->setMsg(std::move(msg));
}

} // namespace fer