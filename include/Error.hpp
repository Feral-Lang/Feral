#pragma once

#include "Utils.hpp"

namespace fer
{

using ModuleId = uint16_t;

// Module ID is the group of 16 most significant bits (2 bytes) of the ModuleLoc which is
// a 64 bit unsigned integer, with the remaining 48 bits denoting the code offset.
// When id is -1, the ModuleLoc is invalid.
struct ModuleLoc
{
	uint64_t id	: 16;
	uint64_t offset : 48;

	ModuleLoc();
	// Note: offset takes a uint64_t, but actually is 48 bits in size.
	ModuleLoc(ModuleId id, uint64_t offset);
};

static_assert(sizeof(ModuleLoc) == sizeof(uint64_t));

class ErrorHandler
{
	Map<ModuleId, String> paths;
	Map<ModuleId, String> codes; // for virtual modules - like <eval>
	size_t maxErrors;

	void outputString(ModuleLoc loc, bool iswarn, const String &e);
	static bool getLineAndColumnFromData(StringRef data, size_t offset, StringRef &line,
					     size_t &lineNum, size_t &column);

	template<typename... Args> void output(ModuleLoc loc, bool iswarn, Args &&...args)
	{
		String res = utils::toString(std::forward<Args>(args)...);
		outputString(loc, iswarn, res);
	}

public:
	ErrorHandler(size_t maxErrors);

	const char *getPathForId(ModuleId id);
	const char *getCodeForId(ModuleId id);

	inline void setPathForId(ModuleId id, StringRef path) { paths.insert_or_assign(id, path); }
	inline void setCodeForId(ModuleId id, String &&code)
	{
		codes.insert_or_assign(id, std::move(code));
	}
	inline void setMaxErrors(size_t maxErr) { maxErrors = maxErr; }

	inline bool moduleIdExists(ModuleId id) { return paths.find(id) != paths.end(); }
	inline bool moduleCodeExists(ModuleId id) { return codes.find(id) != codes.end(); }
	inline size_t getMaxErrors() { return maxErrors; }

	///////////////////////////// Actual error related functions. /////////////////////////////

	template<typename... Args> void fail(ModuleLoc loc, Args &&...args)
	{
		output(loc, false, std::forward<Args>(args)...);
	}

	template<typename... Args> void warn(ModuleLoc loc, Args &&...args)
	{
		output(loc, true, std::forward<Args>(args)...);
	}
};

extern ErrorHandler err;

} // namespace fer