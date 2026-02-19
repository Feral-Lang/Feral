#pragma once

#include "VM/VM.hpp"

namespace fer
{

class VarThread : public Var
{
    String name;
    Var *callable;
    SharedFuture<Var *> *res;
    JThread *thread;
    Vector<Var *> args;
    VarMap *assnArgs;

    void onCreate(VirtualMachine &vm) override;
    void onDestroy(VirtualMachine &vm) override;

public:
    VarThread(ModuleLoc loc, StringRef name, Var *_callable, Vector<Var *> &&_args,
              VarMap *_assnArgs);
    ~VarThread();

    inline StringRef getName() { return name; }
    inline SharedFuture<Var *> *&getFuture() { return res; }
    inline JThread *&getThread() { return thread; }
    inline Thread::id getThreadId() { return thread->get_id(); }
};

} // namespace fer