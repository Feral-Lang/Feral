#include "Logger.hpp"

#include <chrono>

namespace fer
{

Logger logger;

const char *logLevelStr(LogLevels::LogLevels lvl)
{
    if(lvl == LogLevels::FATAL) return "FATAL";
    if(lvl == LogLevels::WARN) return "WARN";
    if(lvl == LogLevels::INFO) return "INFO";
    if(lvl == LogLevels::DEBUG) return "DEBUG";
    if(lvl == LogLevels::TRACE) return "TRACE";

    return "INVALID";
}
const char *logLevelColorStr(LogLevels::LogLevels lvl)
{
    if(lvl == LogLevels::FATAL) return "\033[31m";
    if(lvl == LogLevels::WARN) return "\033[33m";
    if(lvl == LogLevels::INFO) return "\033[35m";
    if(lvl == LogLevels::DEBUG) return "\033[36m";
    if(lvl == LogLevels::TRACE) return "\033[32m";

    return "";
}

SinkInfo::SinkInfo(OStream *f, bool withCol, bool mustClose)
    : f(f), withCol(withCol), mustClose(mustClose)
{}
SinkInfo::SinkInfo(SinkInfo &&si) noexcept : f(si.f), withCol(si.withCol), mustClose(si.mustClose)
{
    si.f         = nullptr;
    si.mustClose = false; // to prevent SinkInfo destructor from calling delete on si.f
}
SinkInfo::~SinkInfo()
{
    if(mustClose) delete f;
}

Logger::Logger() : level(LogLevels::WARN) {}

void Logger::logInternal(LogLevels::LogLevels lvl, StringRef data)
{
    namespace chrono           = std::chrono;
    chrono::microseconds count = std::chrono::duration_cast<std::chrono::microseconds>(
        chrono::system_clock::now().time_since_epoch());
    std::time_t time =
        std::chrono::system_clock::to_time_t(chrono::system_clock::time_point{count});
    std::tm *t        = std::localtime(&time);
    char timeBuf[512] = {0};
    std::strftime(timeBuf, sizeof(timeBuf), "%FT%T%z", t); // %Y-%m-%dT%H:%M:%S+0000
    for(auto &s : sinks) {
        if(s.withCol) {
            (*s.f) << "[" << timeBuf << "][" << logLevelColorStr(lvl) << logLevelStr(lvl)
                   << "\033[0m]: ";
        } else {
            (*s.f) << "[" << timeBuf << "][" << logLevelStr(lvl) << "]: ";
        }
        (*s.f) << data << '\n';
    }
}

bool Logger::addSinkByName(const char *name, bool withCol)
{
    OFStream *f = new OFStream(name);
    if(!f->good()) {
        delete f;
        return false;
    }
    addSink(f, withCol, true);
    return true;
}

} // namespace fer