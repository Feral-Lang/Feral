#pragma once

#include "VM/VarTypes.hpp"

namespace fer
{

class VarMultiProc : public Var
{
	Thread *thread;
	SharedFuture<int> *res;
	size_t id;
	bool owner;

	Var *onCopy(Interpreter &vm, ModuleLoc loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	VarMultiProc(ModuleLoc loc, Thread *thread, SharedFuture<int> *res, bool owner = true);
	VarMultiProc(ModuleLoc loc, Thread *thread, SharedFuture<int> *res, size_t id,
		     bool owner = true);
	~VarMultiProc();

	inline Thread *&getThread() { return thread; }
	inline SharedFuture<int> *&getFuture() { return res; }
	inline size_t getId() { return id; }
};

} // namespace fer