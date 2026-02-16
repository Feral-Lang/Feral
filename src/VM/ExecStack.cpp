#include "VM/ExecStack.hpp"

#include "VM/VM.hpp"

namespace fer
{

ExecStack::ExecStack(VirtualMachine &vm) : vm(vm) { stack.reserve(10); }
ExecStack::~ExecStack()
{
    for(auto &e : stack) vm.decVarRef(e);
}

void ExecStack::push(Var *val, bool iref)
{
    if(iref) vm.incVarRef(val);
    stack.push_back(val);
}
Var *ExecStack::pop(bool dref)
{
    if(stack.empty()) return nullptr;
    Var *back = stack.back();
    stack.pop_back();
    if(dref) vm.decVarRef(back);
    return back;
}

String ExecStack::dump(VirtualMachine *vm)
{
    String outStr;
    for(auto &e : stack) {
        e->dump(outStr, vm);
        outStr += " -- ";
    }
    return outStr;
}

} // namespace fer