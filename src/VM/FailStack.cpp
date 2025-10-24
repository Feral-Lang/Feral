#include "VM/FailStack.hpp"

#include "VM/Interpreter.hpp"

namespace fer
{

FailStack::FailStack(MemoryManager &mem) : mem(mem) {}
FailStack::~FailStack()
{
    assert(size() <= 1 && "Failstack must have <= 1 items before it is destroyed");
    for(auto &s : stack) {
        Var::decVarRef(mem, s);
    }
}

void FailStack::pushHandler(VarFn *handler, size_t popLoc, size_t recurseCount, bool irefHandler)
{
    VarFailure *failure = Var::makeVarWithRef<VarFailure>(mem, handler->getLoc(), handler, popLoc,
                                                          recurseCount, irefHandler);
    stack.push_back(failure);
}

void FailStack::popHandler()
{
    Var::decVarRef(mem, stack.back());
    stack.pop_back();
}

Var *FailStack::handle(VirtualMachine &vm, ModuleLoc loc, size_t &popLoc)
{
    if(stack.empty() || stack.back()->isHandling()) return nullptr;
    VarFailure *f = stack.back();
    Var::incVarRef(f);
    Array<Var *, 2> args{nullptr, f};
    Var *res = f->callHandler(vm, loc, args);
    popLoc   = f->getPopLoc();
    Var::decVarRef(mem, f);
    if(!res) popHandler();
    return res;
}

void FailStack::failStr(ModuleLoc loc, size_t recurseCount, String &&msg)
{
    assert(!stack.empty() && "[Internal error] No element in failstack to handle error");
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