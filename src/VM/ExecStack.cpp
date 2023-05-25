#include "VM/ExecStack.hpp"

namespace fer
{

ExecStack::ExecStack() {}
ExecStack::~ExecStack()
{
	for(auto &e : stack) decref(e);
}

void ExecStack::push(Var *val, bool iref)
{
	if(iref) incref(val);
	stack.push_back(val);
}
Var *ExecStack::pop(bool dref)
{
	if(stack.empty()) return nullptr;
	Var *back = stack.back();
	stack.pop_back();
	if(dref) decref(back);
	return back;
}

} // namespace fer