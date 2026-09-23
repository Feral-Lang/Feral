#include <chrono>
#include <format>

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
           "  fn(timestamp, formatStr) -> Str | Nil\n"
           "Formats the `timestamp` which is in microseconds since epoch, using `formatStr`.\n"
           "Returns the resulting string.\n"
           "Takes the following optional keyword arguments:\n"
           "* `utc = true/false` - if `true`, time will be shown in UTC format, local time "
           "otherwise. (default: false)")
{
    EXPECT(VarInt, args[1], "time");
    EXPECT(VarStr, args[2], "format");
    bool utc = false;
    if(Var *utcVar = assnArgs->getAttr("utc")) {
        EXPECT(VarBool, utcVar, "is utc format");
        utc = as<VarBool>(utcVar)->getVal();
    }
    int64_t val     = as<VarInt>(args[1])->getVal();
    const String &f = as<VarStr>(args[2])->getVal();
    std::chrono::system_clock::time_point tpMicro(std::chrono::microseconds{val});
    auto tpSec{std::chrono::time_point_cast<std::chrono::seconds>(tpMicro)};
    auto tzTime = utc ? std::chrono::zoned_time{"UTC", tpSec}
                      : std::chrono::zoned_time{std::chrono::current_zone(), tpSec};
    auto res    = std::vformat("{:" + f + "}", std::make_format_args(tzTime));
    return vm.makeVar<VarStr>(loc, std::move(res));
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
    vm.addLocal(loc, "parseNative", timeParse);
    vm.addLocal(loc, "formatNative", timeFormat);
    return true;
}

} // namespace fer