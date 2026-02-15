#pragma once

#include "VM/VM.hpp"

namespace fer
{

class VarThread : public Var
{
    String name;
    SharedFuture<Var *> *res;
    JThread *thread;
    VirtualMachine &vm;
    Var *callable;
    Vector<Var *> args;
    StringMap<AssnArgData> assnArgs;

    void onCreate(MemoryManager &mem) override;
    void onDestroy(MemoryManager &mem) override;

public:
    VarThread(ModuleLoc loc, StringRef name, VirtualMachine &_vm, Var *_callable, Span<Var *> _args,
              const StringMap<AssnArgData> &_assnArgs);
    ~VarThread();

    inline StringRef getName() { return name; }
    inline SharedFuture<Var *> *&getFuture() { return res; }
    inline JThread *&getThread() { return thread; }
    inline Thread::id getThreadId() { return thread->get_id(); }
};

} // namespace fer