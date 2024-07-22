#pragma once

#include "VM/Interpreter.hpp"

namespace fer
{

class VarMultiProc : public Var
{
	Thread *thread;
	SharedFuture<int> *res;
	size_t id;
	bool owner;

	Var *copyImpl(const ModuleLoc *loc) override;

public:
	VarMultiProc(const ModuleLoc *loc, Thread *thread, SharedFuture<int> *res,
		     bool owner = true);
	VarMultiProc(const ModuleLoc *loc, Thread *thread, SharedFuture<int> *res, size_t id,
		     bool owner = true);
	~VarMultiProc();

	void set(Var *from);

	inline Thread *&getThread() { return thread; }
	inline SharedFuture<int> *&getFuture() { return res; }
	inline size_t getId() { return id; }
};

} // namespace fer