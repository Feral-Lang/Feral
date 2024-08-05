#include "VM/ExecStack.hpp"

#include "VM/Interpreter.hpp"

namespace fer
{

ExecStack::ExecStack(Interpreter &vm) : vm(vm) {}
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

} // namespace fer