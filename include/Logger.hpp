#pragma once

#include "Utils.hpp"

namespace fer
{

enum class LogLevels
{
	FATAL,
	WARN,
	INFO,
	DEBUG,
	TRACE
};

const char *logLevelStr(LogLevels lvl);
const char *logLevelColorStr(LogLevels lvl);

struct SinkInfo
{
	OStream *f;
	bool with_col;
	bool must_close;

	SinkInfo(OStream *f, bool with_col, bool must_close);
	// declaring a copy constructor deletes implicit move constructor required by vector when it
	// grows
	SinkInfo(SinkInfo &&si) noexcept;
	// no copy constructor
	SinkInfo(const SinkInfo &SinkInfo) = delete;
	// no copy operator
	SinkInfo &operator=(const SinkInfo &SinkInfo) = delete;
	~SinkInfo();
};

class Logger
{
	Vector<SinkInfo> sinks;
	LogLevels level;

	void logInternal(LogLevels lvl, StringRef data);

	template<typename... Args> void log(LogLevels lvl, Args &&...args)
	{
		if((int)level < (int)lvl) return;
		String res = utils::toString(std::forward<Args>(args)...);
		logInternal(lvl, res);
	}

public:
	Logger();

	bool addSinkByName(const char *name, bool with_col);

	inline void addSink(OStream *f, bool with_col, bool must_close)
	{
		sinks.emplace_back(f, with_col, must_close);
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

	inline void setLevel(LogLevels lvl) { level = lvl; }
	inline LogLevels getLevel() { return level; }
};

extern DLL_EXPORT Logger logger;

} // namespace fer