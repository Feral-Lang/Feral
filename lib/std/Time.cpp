#include "VM/VM.hpp"

namespace fer
{

FERAL_FUNC(timePointNow, 0, false,
           "  fn() -> TimePoint\n"
           "Returns the current time as TimePoint.")
{ return vm.makeVar<VarTimePoint>(loc, std::chrono::system_clock::now()); }

FERAL_FUNC(timePointParseNative, 2, false,
           "  fn(timeStr, formatStr) -> TimePoint\n"
           "Parses the time given as `timeStr` using the format given by `formatStr`.\n"
           "Returns the TimePoint as described by the `timeStr`.")
{
    EXPECT(VarStr, args[1], "time");
    EXPECT(VarStr, args[2], "format");
    const char *tmStr  = as<VarStr>(args[1])->cStr();
    const char *fmtStr = as<VarStr>(args[2])->cStr();
    std::tm tm         = {};
    std::stringstream ss(tmStr);
    ss >> std::get_time(&tm, fmtStr);
    if(ss.fail()) {
        vm.fail(loc, "failed to parse the given time string: ", tmStr);
        return nullptr;
    }
    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return vm.makeVar<VarTimePoint>(loc, tp);
}

FERAL_FUNC(durationWeeks, 1, false,
           "  fn(count) -> Duration\n"
           "Returns `count` number of weeks as a Duration.")
{
    int64_t c = as<VarInt>(args[1])->getVal();
    auto d    = std::chrono::duration_cast<Duration>(std::chrono::weeks(c));
    return vm.makeVar<VarDuration>(loc, d);
}

FERAL_FUNC(durationDays, 1, false,
           "  fn(count) -> Duration\n"
           "Returns `count` number of days as a Duration.")
{
    int64_t c = as<VarInt>(args[1])->getVal();
    auto d    = std::chrono::duration_cast<Duration>(std::chrono::days(c));
    return vm.makeVar<VarDuration>(loc, d);
}

FERAL_FUNC(durationHours, 1, false,
           "  fn(count) -> Duration\n"
           "Returns `count` number of hours as a Duration.")
{
    int64_t c = as<VarInt>(args[1])->getVal();
    auto d    = std::chrono::duration_cast<Duration>(std::chrono::hours(c));
    return vm.makeVar<VarDuration>(loc, d);
}

FERAL_FUNC(durationMinutes, 1, false,
           "  fn(count) -> Duration\n"
           "Returns `count` number of minutes as a Duration.")
{
    int64_t c = as<VarInt>(args[1])->getVal();
    auto d    = std::chrono::duration_cast<Duration>(std::chrono::minutes(c));
    return vm.makeVar<VarDuration>(loc, d);
}

FERAL_FUNC(durationSeconds, 1, false,
           "  fn(count) -> Duration\n"
           "Returns `count` number of seconds as a Duration.")
{
    int64_t c = as<VarInt>(args[1])->getVal();
    auto d    = std::chrono::duration_cast<Duration>(std::chrono::seconds(c));
    return vm.makeVar<VarDuration>(loc, d);
}

FERAL_FUNC(durationMilliseconds, 1, false,
           "  fn(count) -> Duration\n"
           "Returns `count` number of milliseconds as a Duration.")
{
    int64_t c = as<VarInt>(args[1])->getVal();
    auto d    = std::chrono::duration_cast<Duration>(std::chrono::milliseconds(c));
    return vm.makeVar<VarDuration>(loc, d);
}

FERAL_FUNC(durationMicroseconds, 1, false,
           "  fn(count) -> Duration\n"
           "Returns `count` number of microseconds as a Duration.")
{
    int64_t c = as<VarInt>(args[1])->getVal();
    auto d    = std::chrono::duration_cast<Duration>(std::chrono::microseconds(c));
    return vm.makeVar<VarDuration>(loc, d);
}

INIT_DLL(Time)
{
    vm.addLocal(loc, "now", timePointNow);
    vm.addLocal(loc, "parseNative", timePointParseNative);
    vm.addLocal(loc, "weeks", durationWeeks);
    vm.addLocal(loc, "days", durationDays);
    vm.addLocal(loc, "hours", durationHours);
    vm.addLocal(loc, "minutes", durationMinutes);
    vm.addLocal(loc, "seconds", durationSeconds);
    vm.addLocal(loc, "milliseconds", durationMilliseconds);
    vm.addLocal(loc, "microseconds", durationMicroseconds);
    return true;
}

} // namespace fer