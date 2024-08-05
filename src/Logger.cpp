#include "Logger.hpp"

#include <chrono>
#include <format>

namespace fer
{

Logger logger;

const char *logLevelStr(LogLevels lvl)
{
	if(lvl == LogLevels::FATAL) return "FATAL";
	if(lvl == LogLevels::WARN) return "WARN";
	if(lvl == LogLevels::INFO) return "INFO";
	if(lvl == LogLevels::DEBUG) return "DEBUG";
	if(lvl == LogLevels::TRACE) return "TRACE";

	return "INVALID";
}
const char *logLevelColorStr(LogLevels lvl)
{
	if(lvl == LogLevels::FATAL) return "\033[31m";
	if(lvl == LogLevels::WARN) return "\033[33m";
	if(lvl == LogLevels::INFO) return "\033[35m";
	if(lvl == LogLevels::DEBUG) return "\033[36m";
	if(lvl == LogLevels::TRACE) return "\033[32m";

	return "";
}

SinkInfo::SinkInfo(OStream *f, bool with_col, bool must_close)
	: f(f), with_col(with_col), must_close(must_close)
{}
SinkInfo::SinkInfo(SinkInfo &&si) noexcept
	: f(si.f), with_col(si.with_col), must_close(si.must_close)
{
	si.f	      = nullptr;
	si.must_close = false; // to prevent SinkInfo destructor from calling delete on si.f
}
SinkInfo::~SinkInfo()
{
	if(must_close) delete f;
}

Logger::Logger() : level(LogLevels::WARN) {}

void Logger::logInternal(LogLevels lvl, StringRef data)
{
	namespace chrono = std::chrono;
	String timebuf =
	std::format("{:%Y-%m-%dT%H:%M:%S%Z}",
		    chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()));

	for(auto &s : sinks) {
		if(s.with_col) {
			(*s.f) << "[" << timebuf << "][" << logLevelColorStr(lvl)
			       << logLevelStr(lvl) << "\033[0m]: ";
		} else {
			(*s.f) << "[" << timebuf << "][" << logLevelStr(lvl) << "]: ";
		}
		(*s.f) << data << '\n';
	}
}

bool Logger::addSinkByName(const char *name, bool with_col)
{
	OFStream *f = new OFStream(name);
	if(!f->good()) {
		delete f;
		return false;
	}
	addSink(f, with_col, true);
	return true;
}

} // namespace fer