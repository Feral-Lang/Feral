#include "Mutex.hpp"

#include "VM/Interpreter.hpp"

namespace fer
{

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// VarMutexBase ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarMutexBase::VarMutexBase(ModuleLoc loc) : Var(loc, 0) {}
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

VarLockGuard::VarLockGuard(ModuleLoc loc, VarMutexBase *mtx) : Var(loc, 0), mtx(mtx) {}
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

FERAL_FUNC(mutexNew, 0, false,
           "  fn() -> Mutex\n"
           "Creates and returns an instance of a Mutex.")
{
    return vm.makeVar<VarMutex>(loc);
}

FERAL_FUNC(recursiveMutexNew, 0, false,
           "  fn() -> RecursiveMutex\n"
           "Creates and returns an instance of a RecursiveMutex.")
{
    return vm.makeVar<VarRecursiveMutex>(loc);
}

FERAL_FUNC(mutexLock, 0, false,
           "  var.fn() -> Nil\n"
           "Locks the mutex `var`.")
{
    as<VarMutexBase>(args[0])->lock();
    return vm.getNil();
}

FERAL_FUNC(mutexTryLock, 0, false,
           "  var.fn() -> Bool\n"
           "Returns `true` if attempting to lock the mutex `var` succeeds.")
{
    return as<VarMutexBase>(args[0])->tryLock() ? vm.getTrue() : vm.getFalse();
}

FERAL_FUNC(mutexUnlock, 0, false,
           "  var.fn() -> Nil\n"
           "Unlocks the mutex `var`.")
{
    as<VarMutexBase>(args[0])->unlock();
    return vm.getNil();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// LockGuard /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(lockguardNew, 1, false,
           "  fn(mutex) -> LockGuard\n"
           "Creates and returns a lock guard, locking the `mutex`.")
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

    vm.registerType<VarMutex>(loc, "Mutex", "Mutex to use for multithreading.");
    vm.registerType<VarRecursiveMutex>(
        loc, "RecursiveMutex",
        "Unlike Mutex, this allows one thread to lock it over and over again.");
    vm.registerType<VarLockGuard>(
        loc, "LockGuard",
        "An object that blocks a mutex and releases it when the object goes out of scope.");

    mod->addNativeFn(vm, "new", mutexNew);
    mod->addNativeFn(vm, "newRecursive", recursiveMutexNew);
    mod->addNativeFn(vm, "newGuard", lockguardNew);

    vm.addNativeTypeFn<VarMutex>(loc, "lock", mutexLock);
    vm.addNativeTypeFn<VarMutex>(loc, "tryLock", mutexTryLock);
    vm.addNativeTypeFn<VarMutex>(loc, "unlock", mutexUnlock);
    vm.addNativeTypeFn<VarRecursiveMutex>(loc, "lock", mutexLock);
    vm.addNativeTypeFn<VarRecursiveMutex>(loc, "tryLock", mutexTryLock);
    vm.addNativeTypeFn<VarRecursiveMutex>(loc, "unlock", mutexUnlock);
    return true;
}

} // namespace fer