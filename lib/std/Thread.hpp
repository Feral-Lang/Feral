#pragma once

#include "VM/VarTypes.hpp"

namespace fer
{

class VarMutex : public Var
{
	Mutex mtx;

public:
	VarMutex(ModuleLoc loc);
	~VarMutex();

	inline Mutex &getMutex() { return mtx; }
};

class VarLockGuard : public Var
{
	LockGuard<Mutex> lg;

public:
	VarLockGuard(ModuleLoc loc, VarMutex *mtx);
	~VarLockGuard();
};

class VarThread : public Var
{
	InterpreterThread *vm;
	SharedFuture<Var *> *res;
	Thread *thread;
	Var *callable;
	Vector<Var *> args;
	StringMap<AssnArgData> assn_args;

public:
	VarThread(ModuleLoc loc, InterpreterThread &currVM, Var *_callable, Span<Var *> _args,
		  const StringMap<AssnArgData> &_assn_args);
	~VarThread();

	inline SharedFuture<Var *> *&getFuture() { return res; }
	inline InterpreterThread *&getVM() { return vm; }
	inline Thread *&getThread() { return thread; }
	inline size_t getThreadId() { return thread->native_handle(); }
};

} // namespace fer