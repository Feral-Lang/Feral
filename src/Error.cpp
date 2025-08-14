/*
	MIT License

	Copyright (c) 2022 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "Error.hpp"

namespace fer
{

ErrorHandler err(10);

ModuleLoc::ModuleLoc()
	: id((ModuleId)-1), offStart(static_cast<ModuleId>(-1)), offEnd(static_cast<ModuleId>(-1))
{}
ModuleLoc::ModuleLoc(ModuleId id, uint64_t offStart, uint64_t offEnd)
	: id(id), offStart(offStart), offEnd(offEnd)
{}

ErrorHandler::ErrorHandler(size_t maxErrors) : maxErrors(maxErrors) {}

void ErrorHandler::addFile(ModuleId id, fs::File *f)
{
	auto pair = files.emplace(id, f);
	paths.emplace(id, pair.first->second->getPath());
}

fs::File *ErrorHandler::getFileForId(ModuleId id)
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