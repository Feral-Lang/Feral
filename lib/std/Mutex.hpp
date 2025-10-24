#pragma once

#include "VM/VarTypes.hpp"

namespace fer
{

class VarMutexBase : public Var
{
public:
    VarMutexBase(ModuleLoc loc);
    virtual ~VarMutexBase() = 0;

    virtual void lock()    = 0;
    virtual bool tryLock() = 0;
    virtual void unlock()  = 0;
};

class VarMutex : public VarMutexBase
{
    Mutex mtx;

public:
    VarMutex(ModuleLoc loc);
    ~VarMutex();

    inline void lock() override { mtx.lock(); }
    inline bool tryLock() override { return mtx.try_lock(); }
    inline void unlock() override { return mtx.unlock(); }

    inline Mutex &getVal() { return mtx; }
};

class VarRecursiveMutex : public VarMutexBase
{
    RecursiveMutex mtx;

public:
    VarRecursiveMutex(ModuleLoc loc);
    ~VarRecursiveMutex();

    inline void lock() override { mtx.lock(); }
    inline bool tryLock() override { return mtx.try_lock(); }
    inline void unlock() override { return mtx.unlock(); }

    inline RecursiveMutex &getVal() { return mtx; }
};

class VarLockGuard : public Var
{
    VarMutexBase *mtx;

    void onCreate(MemoryManager &mem) override;
    void onDestroy(MemoryManager &mem) override;

public:
    VarLockGuard(ModuleLoc loc, VarMutexBase *mtx);
    ~VarLockGuard();
};

} // namespace fer