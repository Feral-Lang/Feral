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
    uint64_t id       : 16;
    uint64_t offStart : 24;
    uint64_t offEnd   : 24;

    ModuleLoc();
    // Note: off{Start,End} take uint64_t, but are actually 24 bits each in size.
    ModuleLoc(ModuleId id, uint64_t offStart, uint64_t offEnd);

    inline bool operator==(const ModuleLoc &other) const
    {
        return id == other.id && offStart == other.offStart && offEnd == other.offEnd;
    }
    inline bool operator!=(const ModuleLoc &other) const { return !(*this == other); }
};

static_assert(sizeof(ModuleLoc) == sizeof(uint64_t));

class ErrorHandler
{
    Map<ModuleId, StringRef> paths;
    Map<ModuleId, fs::File *> files;

public:
    void addFile(ModuleId id, fs::File *f);

    fs::File *getFileForId(ModuleId id);
    StringRef getPathForId(ModuleId id);

    inline bool moduleIdExists(ModuleId id) { return paths.contains(id); }

    ///////////////////////////// Actual error related functions. /////////////////////////////

    inline void outStr(ModuleLoc loc, StringRef data = "")
    {
        utils::output(std::cerr, getFileForId(loc.id), loc.offStart, loc.offEnd, data);
    }

    template<typename... Args> void fail(ModuleLoc loc, Args &&...args)
    {
        String s = utils::toString("Error: ", std::forward<Args>(args)...);
        outStr(loc, s);
    }
};

extern DLL_EXPORT ErrorHandler err;

} // namespace fer