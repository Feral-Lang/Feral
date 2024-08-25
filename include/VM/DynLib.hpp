#pragma once

// Wrapper class for dlfcn.h library

#include "Core.hpp"

namespace fer
{

class DynLib
{
	StringMap<void *> handles;

public:
	DynLib();
	~DynLib();

	static DynLib &getInstance();

	// ensure the source of filepath StringRef does NOT get deleted before DynLib instance
	void *load(const char *filepath);
	void unload(StringRef filepath);
	void *get(StringRef filepath, const char *sym);

	inline bool exists(StringRef filepath) { return handles.find(filepath) != handles.end(); }
};

} // namespace fer