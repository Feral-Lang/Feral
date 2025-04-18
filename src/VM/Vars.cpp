#include "VM/Vars.hpp"

namespace fer
{

VarFrame::VarFrame(MemoryManager &mem) : mem(mem) {}
VarFrame::~VarFrame()
{
	for(auto &v : vars) Var::decVarRef(mem, v.second);
}

Var *VarFrame::get(StringRef name)
{
	auto loc = vars.find(name);
	if(loc == vars.end()) return nullptr;
	return loc->second;
}

void VarFrame::dump(OStream &os, size_t tab)
{
	String tabs(tab, '\t');
	os << tabs << "VarFrame\n";
	tabs += '\t';
	for(auto it : vars) {
		os << tabs << it.first << "\n";
	}
}

void VarFrame::add(StringRef name, Var *val, bool iref)
{
	auto loc = vars.find(name);
	if(loc != vars.end()) Var::decVarRef(mem, loc->second);
	if(iref) Var::incVarRef(val);
	vars.insert_or_assign(String(name), val);
}
bool VarFrame::rem(StringRef name, bool dref)
{
	auto loc = vars.find(name);
	if(loc == vars.end()) return false;
	if(dref) Var::decVarRef(mem, loc->second);
	vars.erase(loc);
	return true;
}
VarFrame *VarFrame::create(MemoryManager &mem) { return mem.alloc<VarFrame>(mem); }
void VarFrame::destroy(MemoryManager &mem, VarFrame *frame)
{
	frame->~VarFrame();
	mem.free(frame);
}

VarStack::VarStack(MemoryManager &mem) : mem(mem) { pushStack(1); }
VarStack::~VarStack() { popStack(stack.size()); }

void VarStack::pushStack(size_t count)
{
	LockGuard<RecursiveMutex> _(mtx);
	for(size_t i = 0; i < count; ++i) stack.push_back(VarFrame::create(mem));
}
void VarStack::popStack(size_t count)
{
	LockGuard<RecursiveMutex> _(mtx);
	for(size_t i = 0; i < count; ++i) {
		VarFrame::destroy(mem, stack.back());
		stack.pop_back();
	}
}

Var *VarStack::get(StringRef name)
{
	LockGuard<RecursiveMutex> _(mtx);
	for(auto layer = stack.rbegin(); layer != stack.rend(); ++layer) {
		Var *res = (*layer)->get(name);
		if(res) return res;
	}
	return nullptr;
}

void VarStack::pushLoop()
{
	LockGuard<RecursiveMutex> _(mtx);
	loops_from.push_back(stack.size());
	pushStack(1);
}
void VarStack::popLoop()
{
	LockGuard<RecursiveMutex> _(mtx);
	assert(loops_from.size() > 0 && "Cannot VarStack::popLoop() from an empty loop stack");
	if(stack.size() - 1 >= loops_from.back()) {
		popStack(stack.size() - loops_from.back());
	}
	loops_from.pop_back();
}
void VarStack::continueLoop()
{
	LockGuard<RecursiveMutex> _(mtx);
	assert(loops_from.size() > 0 && "Cannot VarStack::popLoop() from an empty loop stack");
	if(stack.size() - 1 > loops_from.back()) popStack(stack.size() - 1 - loops_from.back());
}

bool VarStack::rem(StringRef name, bool dref)
{
	LockGuard<RecursiveMutex> _(mtx);
	for(auto layer = stack.rbegin(); layer != stack.rend(); ++layer) {
		if((*layer)->rem(name, dref)) return true;
	}
	return false;
}

void VarStack::dump(OStream &os, size_t tab)
{
	String tabs(tab, '\t');
	os << tabs << "VarStack\n";
	for(auto &item : stack) {
		item->dump(os, tab + 1);
	}
}

Vars::Vars(MemoryManager &mem) : fnstack(-1), mem(mem) {}
Vars::~Vars() { assert(fnstack == -1); }

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

void Vars::pushModScope(VarStack *modScope)
{
	modScopeStack.push_back(modScope);
	fnvars[0] = modScope;
	if(fnstack == -1) fnstack = 0;
}
void Vars::popModScope()
{
	assert(modScopeStack.size() > 0);
	modScopeStack.pop_back();
	if(modScopeStack.empty()) {
		fnvars[0] = nullptr;
		fnstack	  = -1;
	} else {
		fnvars[0] = modScopeStack.back();
	}
}

void Vars::pushFn()
{
	++fnstack;
	if(fnstack == 0) return;
	fnvars[fnstack] = new VarStack(mem);
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
	if(iref) Var::incVarRef(val);
	stashed.insert({String(name), val});
}
void Vars::unstash()
{
	for(auto &s : stashed) Var::decVarRef(mem, s.second);
	stashed.clear();
}

void Vars::dump(OStream &os)
{
	for(auto it : fnvars) {
		os << "-- In Vars (" << it.first << "):\n";
		it.second->dump(os, 1);
	}
}

Vars::ScopedModScope::ScopedModScope(Vars &vars, VarStack *modScope) : vars(vars)
{
	vars.pushModScope(modScope);
}
Vars::ScopedModScope::~ScopedModScope() { vars.popModScope(); }

} // namespace fer