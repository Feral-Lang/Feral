#pragma once

#include "VarTypes.hpp"

namespace fer
{

class Interpreter;
class VarFrame
{
	StringMap<Var *> vars;
	Interpreter &vm;

public:
	VarFrame(Interpreter &vm);
	~VarFrame();

	inline StringMap<Var *> &get() { return vars; }
	inline bool exists(StringRef name) { return vars.find(name) != vars.end(); }

	// use this instead of exists() if the Var* retrieval is actually required
	Var *get(StringRef name);

	void add(StringRef name, Var *val, bool iref);
	bool rem(StringRef name, bool dref);
};

class VarStack
{
	Vector<size_t> loops_from;
	// each VarFrame is a stack frame
	// Vector is not used here as VarFrame has to be stored as a pointer.
	// This is so because otherwise, on vector resize, it will cause the VarFrame object to
	// delete and reconstruct, therefore incorrectly calling the dref() calls
	Vector<VarFrame *> stack;
	Interpreter &vm;

public:
	VarStack(Interpreter &vm);
	~VarStack();

	void pushStack(size_t count);
	void popStack(size_t count);

	inline bool exists(StringRef name) { return stack.back()->exists(name); }
	// use this instead of exists() if the Var* retrieval is actually required
	Var *get(StringRef name);

	void pushLoop();
	// 'break' also uses this
	void popLoop();
	void continueLoop();

	inline size_t size() { return stack.size(); }
	inline void resizeTo(size_t count)
	{
		if(stack.size() > count) popStack(stack.size() - count);
	}
	inline void add(StringRef name, Var *val, bool iref) { stack.back()->add(name, val, iref); }
	bool rem(StringRef name, bool dref);
};

class Vars
{
	StringMap<Var *> stashed;
	// maps function ids to VarStack
	// 0 is the id for global scope
	Map<size_t, VarStack *> fnvars;
	size_t fnstack;
	Interpreter &vm;

public:
	Vars(Interpreter &vm);
	~Vars();

	// checks if variable exists in current scope ONLY
	inline bool exists(StringRef name) { return fnvars[fnstack]->exists(name); }
	// use this instead of exists() if the Var* retrieval is actually required
	// and current scope requirement is not present
	Var *get(StringRef name);

	void pushBlk(size_t count);
	inline void popBlk(size_t count) { fnvars[fnstack]->popStack(count); }
	inline size_t getBlkSize() { return fnvars[fnstack]->size(); }
	inline void resizeBlkTo(size_t count) { fnvars[fnstack]->resizeTo(count); }

	void pushFn();
	void popFn();
	void stash(StringRef name, Var *val, bool iref = true);
	void unstash();

	inline void pushLoop() { fnvars[fnstack]->pushLoop(); }
	inline void popLoop() { fnvars[fnstack]->popLoop(); }
	inline void continueLoop() { fnvars[fnstack]->continueLoop(); }

	inline void add(StringRef name, Var *val, bool iref)
	{
		fnvars[fnstack]->add(name, val, iref);
	}
	// add variable to module level unconditionally (for vm.registerNewType())
	inline void addm(StringRef name, Var *val, bool iref) { fnvars[0]->add(name, val, iref); }
	inline bool rem(StringRef name, bool dref) { return fnvars[fnstack]->rem(name, dref); }
};

} // namespace fer