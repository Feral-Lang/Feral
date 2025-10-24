#include <chrono>

#include "VM/Interpreter.hpp"

namespace fer
{

Var *sysclkNow(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
               const StringMap<AssnArgData> &assn_args)
{
    VarInt *res = vm.makeVar<VarInt>(loc, 0);
    // Not using nanoseconds because that's the same count of digits as int64_t::max
    res->setVal(std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count());
    return res;
}

Var *formatTime(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                const StringMap<AssnArgData> &assn_args)
{
    if(!args[1]->is<VarInt>()) {
        vm.fail(loc, "expected integer argument as time for formatting, found: ",
                vm.getTypeName(args[1]));
        return nullptr;
    }
    if(!args[2]->is<VarStr>()) {
        vm.fail(loc, "expected string argument as format for time formatting, found: ",
                vm.getTypeName(args[2]));
        return nullptr;
    }
    uint64_t val = as<VarInt>(args[1])->getVal();
    std::chrono::microseconds usval(val);
    std::chrono::system_clock::time_point tp(std::chrono::microseconds{usval});
    std::time_t time = std::chrono::system_clock::to_time_t(tp);
    std::tm *t       = std::localtime(&time);
    char fmt[1024]   = {0};
    if(std::strftime(fmt, sizeof(fmt), as<VarStr>(args[2])->getVal().c_str(), t)) {
        return vm.makeVar<VarStr>(loc, fmt);
    }
    return vm.getNil();
}

INIT_MODULE(Time)
{
    VarModule *mod = vm.getCurrModule();
    mod->addNativeFn(vm, "systemClockNowNative", sysclkNow);
    mod->addNativeFn(vm, "formatNative", formatTime, 2);
    return true;
}

} // namespace fer