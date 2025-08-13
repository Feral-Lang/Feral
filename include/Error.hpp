#pragma once

#include "Core.hpp"

namespace fer
{

using ModuleId = uint16_t;

// Module ID is the group of 16 most significant bits (2 bytes) of the ModuleLoc which is
// a 64 bit unsigned integer, with the remaining 48 bits denoting the code offset - start and end
// (24 each). When id is -1, the ModuleLoc is invalid.
struct ModuleLoc
{
	uint64_t id	  : 16;
	uint64_t offStart : 24;
	uint64_t offEnd	  : 24;

	ModuleLoc();
	// Note: off{Start,End} take uint64_t, but are actually 24 bits each in size.
	ModuleLoc(ModuleId id, uint64_t offStart, uint64_t offEnd);
};

static_assert(sizeof(ModuleLoc) == sizeof(uint64_t));

class ErrorHandler
{
	Map<ModuleId, StringRef> paths;
	Map<ModuleId, fs::File> files;
	size_t maxErrors;

public:
	ErrorHandler(size_t maxErrors);

	void addFile(ModuleId id, fs::File &&f);

	fs::File *getFileForId(ModuleId id);
	StringRef getPathForId(ModuleId id);

	inline bool moduleIdExists(ModuleId id) { return paths.contains(id); }
	inline void setMaxErrors(size_t maxErr) { maxErrors = maxErr; }
	inline size_t getMaxErrors() { return maxErrors; }

	///////////////////////////// Actual error related functions. /////////////////////////////

	template<typename... Args> void fail(ModuleLoc loc, Args &&...args)
	{
		utils::output(getFileForId(loc.id), std::cerr, loc.offStart, loc.offEnd, false,
			      std::forward<Args>(args)...);
	}

	template<typename... Args> void warn(ModuleLoc loc, Args &&...args)
	{
		utils::output(getFileForId(loc.id), std::cerr, loc.offStart, loc.offEnd, true,
			      std::forward<Args>(args)...);
	}
};

extern DLL_EXPORT ErrorHandler err;

} // namespace fer