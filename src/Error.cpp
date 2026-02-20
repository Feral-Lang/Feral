#include "Error.hpp"

namespace fer
{

ErrorHandler err;

ModuleLoc::ModuleLoc()
    : id((ModuleId)-1), offStart(static_cast<ModuleId>(-1)), offEnd(static_cast<ModuleId>(-1))
{}
ModuleLoc::ModuleLoc(ModuleId id, uint64_t offStart, uint64_t offEnd)
    : id(id), offStart(offStart), offEnd(offEnd)
{}

void ErrorHandler::addFile(ModuleId id, File *f)
{
    auto pair = files.emplace(id, f);
    paths.emplace(id, pair.first->second->getPath());
}

File *ErrorHandler::getFileForId(ModuleId id)
{
    auto loc = files.find(id);
    if(loc == files.end()) return nullptr;
    return loc->second;
}
StringRef ErrorHandler::getPathForId(ModuleId id)
{
    auto loc = paths.find(id);
    if(loc == paths.end()) return "";
    return loc->second;
}

} // namespace fer