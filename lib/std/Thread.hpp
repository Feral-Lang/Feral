#pragma once

#include "VM/Interpreter.hpp"

namespace fer
{

class VarThread : public Var
{
	String name;
	SharedFuture<Var *> *res;
	JThread *thread;
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
	inline JThread *&getThread() { return thread; }
	inline Thread::id getThreadId() { return thread->get_id(); }
};

} // namespace fer