#include "Mutex.hpp"

#include "VM/Interpreter.hpp"

namespace fer
{

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// VarMutexBase ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarMutexBase::VarMutexBase(ModuleLoc loc) : Var(loc, false, false) {}
VarMutexBase::~VarMutexBase() {}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// VarMutex //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarMutex::VarMutex(ModuleLoc loc) : VarMutexBase(loc) {}
VarMutex::~VarMutex() {}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// VarRecursiveMutex //////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarRecursiveMutex::VarRecursiveMutex(ModuleLoc loc) : VarMutexBase(loc) {}
VarRecursiveMutex::~VarRecursiveMutex() {}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// VarLockGuard ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarLockGuard::VarLockGuard(ModuleLoc loc, VarMutexBase *mtx) : Var(loc, false, false), mtx(mtx) {}
VarLockGuard::~VarLockGuard() {}

void VarLockGuard::onCreate(MemoryManager &mem)
{
    incVarRef(mtx);
    mtx->lock();
}
void VarLockGuard::onDestroy(MemoryManager &mem)
{
    mtx->unlock();
    decVarRef(mem, mtx);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Mutexes //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *mutexNew(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
              const StringMap<AssnArgData> &assnArgs)
{
    return vm.makeVar<VarMutex>(loc);
}

Var *recursiveMutexNew(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                       const StringMap<AssnArgData> &assnArgs)
{
    return vm.makeVar<VarRecursiveMutex>(loc);
}

Var *mutexLock(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
               const StringMap<AssnArgData> &assnArgs)
{
    as<VarMutexBase>(args[0])->lock();
    return vm.getNil();
}

Var *mutexTryLock(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                  const StringMap<AssnArgData> &assnArgs)
{
    return as<VarMutexBase>(args[0])->tryLock() ? vm.getTrue() : vm.getFalse();
}

Var *mutexUnlock(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                 const StringMap<AssnArgData> &assnArgs)
{
    as<VarMutexBase>(args[0])->unlock();
    return vm.getNil();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// LockGuard /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *lockGuardNew(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                  const StringMap<AssnArgData> &assnArgs)
{
    if(!args[1]->isDerivedFrom<VarMutexBase>()) {
        vm.fail(loc, "expected a mutex argument for creating a lockguard, found: ",
                vm.getTypeName(args[1]));
        return nullptr;
    }
    return vm.makeVar<VarLockGuard>(loc, as<VarMutexBase>(args[1]));
}

INIT_MODULE(Mutex)
{
    VarModule *mod = vm.getCurrModule();

    vm.registerType<VarMutex>(loc, "Mutex");
    vm.registerType<VarRecursiveMutex>(loc, "RecursiveMutex");
    vm.registerType<VarLockGuard>(loc, "LockGuard");

    mod->addNativeFn(vm, "new", mutexNew);
    mod->addNativeFn(vm, "newRecursive", recursiveMutexNew);
    mod->addNativeFn(vm, "newGuard", lockGuardNew, 1);

    vm.addNativeTypeFn<VarMutex>(loc, "lock", mutexLock, 0);
    vm.addNativeTypeFn<VarMutex>(loc, "tryLock", mutexTryLock, 0);
    vm.addNativeTypeFn<VarMutex>(loc, "unlock", mutexUnlock, 0);
    vm.addNativeTypeFn<VarRecursiveMutex>(loc, "lock", mutexLock, 0);
    vm.addNativeTypeFn<VarRecursiveMutex>(loc, "tryLock", mutexTryLock, 0);
    vm.addNativeTypeFn<VarRecursiveMutex>(loc, "unlock", mutexUnlock, 0);
    return true;
}

} // namespace fer