#include "VM/FailStack.hpp"

namespace fer
{
FailStack::FailStack() {}
FailStack::~FailStack()
{
	assert(stack.empty() && "Expected fail stack to be empty, but it is not");
}

void FailStack::push(Var *var, bool iref)
{
	if(iref) incref(var);
	stack.back().push_back(var);
}
Var *FailStack::pop(bool dref)
{
	if(stack.empty() || stack.back().empty()) return nullptr;
	Var *front = stack.back().front();
	stack.back().pop_front();
	if(dref) decref(front);
	return front;
}

} // namespace fer