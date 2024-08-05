#include "VM/FailStack.hpp"

#include "VM/Interpreter.hpp"

namespace fer
{
FailStack::FailStack(Interpreter &vm) : vm(vm) {}
FailStack::~FailStack()
{
	assert(stack.empty() && "Expected fail stack to be empty, but it is not");
}

void FailStack::initFrame(size_t recurseLevel, StringRef varName, size_t blkBegin, size_t blkEnd)
{
	stack.back().usable	  = true;
	stack.back().recurseLevel = recurseLevel;
	stack.back().varName	  = varName;
	stack.back().blkBegin	  = blkBegin;
	stack.back().blkEnd	  = blkEnd;
	stack.back().errMsg	  = nullptr;
}

void FailStack::reset()
{
	if(stack.back().errMsg) vm.decVarRef(stack.back().errMsg);
	stack.back() = {};
}

} // namespace fer