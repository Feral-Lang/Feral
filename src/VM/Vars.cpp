#include "VM/Vars.hpp"

#include "VM/InterpreterState.hpp"

namespace fer
{

VarFrame::VarFrame(InterpreterState &vms) : vms(vms) {}
VarFrame::~VarFrame()
{
	for(auto &v : vars) vms.decVarRef(v.second);
}

Var *VarFrame::get(StringRef name)
{
	auto loc = vars.find(name);
	if(loc == vars.end()) return nullptr;
	return loc->second;
}

void VarFrame::add(StringRef name, Var *val, bool iref)
{
	auto loc = vars.find(name);
	if(loc != vars.end()) vms.decVarRef(loc->second);
	if(iref) vms.incVarRef(val);
	vars.insert_or_assign(String(name), val);
}
bool VarFrame::rem(StringRef name, bool dref)
{
	auto loc = vars.find(name);
	if(loc == vars.end()) return false;
	if(dref) vms.decVarRef(loc->second);
	vars.erase(loc);
	return true;
}

VarStack::VarStack(InterpreterState &vms) : vms(vms) { pushStack(1); }
VarStack::~VarStack()
{
	for(auto layer = stack.rbegin(); layer != stack.rend(); ++layer) delete *layer;
}

void VarStack::pushStack(size_t count)
{
	for(size_t i = 0; i < count; ++i) stack.push_back(new VarFrame(vms));
}
void VarStack::popStack(size_t count)
{
	for(size_t i = 0; i < count; ++i) {
		delete stack.back();
		stack.pop_back();
	}
}

Var *VarStack::get(StringRef name)
{
	for(auto layer = stack.rbegin(); layer != stack.rend(); ++layer) {
		Var *res = (*layer)->get(name);
		if(res) return res;
	}
	return nullptr;
}

void VarStack::pushLoop()
{
	loops_from.push_back(stack.size());
	stack.push_back(new VarFrame(vms));
}
void VarStack::popLoop()
{
	assert(loops_from.size() > 0 && "Cannot VarStack::popLoop() from an empty loop stack");
	if(stack.size() - 1 >= loops_from.back()) {
		popStack(stack.size() - loops_from.back());
	}
	loops_from.pop_back();
}
void VarStack::continueLoop()
{
	assert(loops_from.size() > 0 && "Cannot VarStack::popLoop() from an empty loop stack");
	if(stack.size() - 1 > loops_from.back()) popStack(stack.size() - 1 - loops_from.back());
}

bool VarStack::rem(StringRef name, bool dref)
{
	for(auto layer = stack.rbegin(); layer != stack.rend(); ++layer) {
		if((*layer)->rem(name, dref)) return true;
	}
	return false;
}

Vars::Vars(InterpreterState &vms) : fnstack(-1), vms(vms) { fnvars[0] = new VarStack(vms); }
Vars::~Vars()
{
	assert(fnstack == 0 || fnstack == -1);
	delete fnvars[0];
}

Var *Vars::get(StringRef name)
{
	assert(fnstack != -1);
	Var *res = fnvars[fnstack]->get(name);
	if(res == nullptr && fnstack != 0) {
		res = fnvars[0]->get(name);
	}
	return res;
}

void Vars::pushBlk(size_t count)
{
	fnvars[fnstack]->pushStack(count);
	for(auto &s : stashed) fnvars[fnstack]->add(s.first, s.second, false);
	stashed.clear();
}

void Vars::pushFn()
{
	++fnstack;
	if(fnstack == 0) return;
	fnvars[fnstack] = new VarStack(vms);
}
void Vars::popFn()
{
	if(fnstack == 0) return;
	auto loc = fnvars.find(fnstack);
	delete loc->second;
	fnvars.erase(loc);
	--fnstack;
}
void Vars::stash(StringRef name, Var *val, bool iref)
{
	if(iref) vms.incVarRef(val);
	stashed.insert({String(name), val});
}
void Vars::unstash()
{
	for(auto &s : stashed) vms.decVarRef(s.second);
	stashed.clear();
}

} // namespace fer