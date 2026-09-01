#include <chrono>

#include "VM/VM.hpp"

namespace fer
{

FERAL_FUNC(timeNow, 0, false,
           "  fn() -> Int\n"
           "Returns the current time in microseconds since epoch.")
{
    VarInt *res = vm.makeVar<VarInt>(loc, 0);
    // Not using nanoseconds because that's the same count of digits as int64_t::max
    res->setVal(std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count());
    return res;
}

FERAL_FUNC(timeFormat, 2, false,
           "  fn(timestamp, format) -> Str | Nil\n"
           "Formats the `timestamp` which is in microseconds since epoch, using `format` string, "
           "returning the resulting string.")
{
    EXPECT(VarInt, args[1], "time");
    EXPECT(VarStr, args[2], "format");
    int64_t val     = as<VarInt>(args[1])->getVal();
    const String &f = as<VarStr>(args[2])->getVal();
    std::chrono::system_clock::time_point tp(std::chrono::microseconds{val});
    std::time_t time = std::chrono::system_clock::to_time_t(tp);
    std::tm *t       = std::localtime(&time);
    char fmt[1024]   = {0};
    if(std::strftime(fmt, sizeof(fmt), f.c_str(), t)) return vm.makeVar<VarStr>(loc, fmt);
    return vm.getNil();
}

FERAL_FUNC(timeParse, 2, false,
           "  fn(timeStr, formatStr) -> Int | Nil\n"
           "Parses the time given as `timeStr` using the format given by `formatStr`.\n"
           "Returns the number of microseconds since epoch, described by the `timeStr`.")
{
    EXPECT(VarStr, args[1], "time");
    EXPECT(VarStr, args[2], "format");
    const String &tm  = as<VarStr>(args[1])->getVal();
    const String &fmt = as<VarStr>(args[2])->getVal();
    std::istringstream is{tm};
    std::chrono::local_time<std::chrono::microseconds> t;
    is.imbue(std::locale(""));
    is >> std::chrono::parse(fmt.c_str(), t);
    if(is.fail()) return vm.getNil();
    return vm.makeVar<VarInt>(loc, t.time_since_epoch().count());
}

INIT_DLL(Time)
{
    vm.addLocal(loc, "now", timeNow);
    vm.addLocal(loc, "parse", timeParse);
    vm.addLocal(loc, "formatNative", timeFormat);
    return true;
}

} // namespace fer