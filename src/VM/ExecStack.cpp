#include "VM/ExecStack.hpp"

namespace fer
{

ExecStack::ExecStack(MemoryManager &mem) : mem(mem) {}
ExecStack::~ExecStack()
{
    for(auto &e : stack) Var::decVarRef(mem, e);
}

void ExecStack::push(Var *val, bool iref)
{
    if(iref) Var::incVarRef(val);
    stack.push_back(val);
}
Var *ExecStack::pop(bool dref)
{
    if(stack.empty()) return nullptr;
    Var *back = stack.back();
    stack.pop_back();
    if(dref) Var::decVarRef(mem, back);
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