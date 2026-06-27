#pragma once

#include "Utils.hpp"

namespace fer
{

namespace LogLevels
{
enum LogLevels
{
    FATAL,
    WARN,
    INFO,
    DEBUG,
    TRACE
};
} // namespace LogLevels

FER_API const char *logLevelStr(LogLevels::LogLevels lvl);
FER_API const char *logLevelColorStr(LogLevels::LogLevels lvl);

struct SinkInfo
{
    OStream *f;
    bool withCol;
    bool mustClose;

    SinkInfo(OStream *f, bool withCol, bool mustClose);
    // Declaring a copy constructor deletes implicit move constructor required by vector when it
    // grows.
    SinkInfo(SinkInfo &&si) noexcept;
    // No copy constructor and copy operator.
    SinkInfo(const SinkInfo &SinkInfo)            = delete;
    SinkInfo &operator=(const SinkInfo &SinkInfo) = delete;
    ~SinkInfo();
};

class FER_API Logger
{
    Vector<SinkInfo> sinks;
    LogLevels::LogLevels level;

    void logInternal(LogLevels::LogLevels lvl, StringRef data);

    template<typename... Args> void log(LogLevels::LogLevels lvl, Args &&...args)
    {
        if(!isLevelLoggable(lvl)) return;
        String res = utils::toString(std::forward<Args>(args)...);
        logInternal(lvl, res);
    }

public:
    Logger();
    // No copy constructor and copy operator.
    Logger(const Logger &logger)            = delete;
    Logger &operator=(const Logger &logger) = delete;

    bool addSinkByName(const char *name, bool withCol);

    inline void addSink(OStream *f, bool withCol, bool mustClose)
    {
        sinks.emplace_back(f, withCol, mustClose);
    }

    template<typename... Args> void fatal(Args &&...args)
    {
        log(LogLevels::FATAL, std::forward<Args>(args)...);
    }
    template<typename... Args> void warn(Args &&...args)
    {
        log(LogLevels::WARN, std::forward<Args>(args)...);
    }
    template<typename... Args> void info(Args &&...args)
    {
        log(LogLevels::INFO, std::forward<Args>(args)...);
    }
    template<typename... Args> void debug(Args &&...args)
    {
        log(LogLevels::DEBUG, std::forward<Args>(args)...);
    }
    template<typename... Args> void trace(Args &&...args)
    {
        log(LogLevels::TRACE, std::forward<Args>(args)...);
    }

    inline void setLevel(LogLevels::LogLevels lvl) { level = lvl; }
    inline LogLevels::LogLevels getLevel() { return level; }
    inline bool isLevelLoggable(LogLevels::LogLevels lvl) { return level >= lvl; }
};

extern FER_API Logger logger;

#define LOG_OBJ_FATAL(loggerObj, ...)                                                     \
    do {                                                                                  \
        if(loggerObj.isLevelLoggable(LogLevels::FATAL)) { loggerObj.fatal(__VA_ARGS__); } \
    } while(false)
#define LOG_OBJ_WARN(loggerObj, ...)                                                    \
    do {                                                                                \
        if(loggerObj.isLevelLoggable(LogLevels::WARN)) { loggerObj.warn(__VA_ARGS__); } \
    } while(false)
#define LOG_OBJ_INFO(loggerObj, ...)                                                    \
    do {                                                                                \
        if(loggerObj.isLevelLoggable(LogLevels::INFO)) { loggerObj.info(__VA_ARGS__); } \
    } while(false)
#define LOG_OBJ_DEBUG(loggerObj, ...)                                                     \
    do {                                                                                  \
        if(loggerObj.isLevelLoggable(LogLevels::DEBUG)) { loggerObj.debug(__VA_ARGS__); } \
    } while(false)
#define LOG_OBJ_TRACE(loggerObj, ...)                                                     \
    do {                                                                                  \
        if(loggerObj.isLevelLoggable(LogLevels::TRACE)) { loggerObj.trace(__VA_ARGS__); } \
    } while(false)

#define LOG_FATAL(...) LOG_OBJ_FATAL(::fer::logger, __VA_ARGS__)
#define LOG_WARN(...) LOG_OBJ_WARN(::fer::logger, __VA_ARGS__)
#define LOG_INFO(...) LOG_OBJ_INFO(::fer::logger, __VA_ARGS__)
#define LOG_DEBUG(...) LOG_OBJ_DEBUG(::fer::logger, __VA_ARGS__)
#define LOG_TRACE(...) LOG_OBJ_TRACE(::fer::logger, __VA_ARGS__)

} // namespace fer