#pragma once

#include "VM/Interpreter.hpp"

namespace fer
{

class VarAtomicBool : public Var
{
	Atomic<bool> val;

	Var *onCopy(MemoryManager &mem, ModuleLoc loc) override;
	void onSet(MemoryManager &mem, Var *from) override;

public:
	VarAtomicBool(ModuleLoc loc, bool _val);

	inline void setVal(bool newval) { val.store(newval); }
	inline bool getVal() { return val.load(); }
};

class VarAtomicInt : public Var
{
	Atomic<int64_t> val;

	Var *onCopy(MemoryManager &mem, ModuleLoc loc) override;
	void onSet(MemoryManager &mem, Var *from) override;

public:
	VarAtomicInt(ModuleLoc loc, int64_t _val);
	VarAtomicInt(ModuleLoc loc, const char *_val);

	inline void setVal(int64_t newval) { val.store(newval); }
	inline int64_t getVal() { return val.load(); }
};

class VarMutex : public Var
{
	RecursiveMutex mtx;

public:
	VarMutex(ModuleLoc loc);
	~VarMutex();

	inline void lock() { mtx.lock(); }
	inline bool tryLock() { return mtx.try_lock(); }
	inline void unlock() { return mtx.unlock(); }
	inline RecursiveMutex &getMutex() { return mtx; }
};

class VarLockGuard : public Var
{
	VarMutex *mtx;

	void onCreate(MemoryManager &mem) override;
	void onDestroy(MemoryManager &mem) override;

public:
	VarLockGuard(ModuleLoc loc, VarMutex *mtx);
	~VarLockGuard();
};

class VarThread : public Var
{
	String name;
	SharedFuture<Var *> *res;
	Thread *thread;
	Interpreter &ip;
	Var *callable;
	Vector<Var *> args;
	StringMap<AssnArgData> assn_args;

	void onCreate(MemoryManager &mem) override;
	void onDestroy(MemoryManager &mem) override;

public:
	VarThread(ModuleLoc loc, StringRef name, Interpreter &_ip, Var *_callable,
		  Span<Var *> _args, const StringMap<AssnArgData> &_assn_args);
	~VarThread();

	inline StringRef getName() { return name; }
	inline SharedFuture<Var *> *&getFuture() { return res; }
	inline Thread *&getThread() { return thread; }
	inline Thread::id getThreadId() { return thread->get_id(); }
};

} // namespace fer