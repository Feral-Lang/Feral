#include "VM/DynLib.hpp"

#include <dlfcn.h>

#include "Error.hpp"
#include "FS.hpp"

namespace fer
{

DynLib::DynLib() {}
DynLib::~DynLib()
{
	for(auto &e : handles) {
		if(e.second != nullptr) dlclose(e.second);
	}
}

DynLib &DynLib::getInstance()
{
	static DynLib dynlib;
	return dynlib;
}

void *DynLib::load(StringRef filepath)
{
	auto handle = handles.find(filepath);
	if(handle != handles.end()) return handle->second;

	// I WANNA USE STRINGREF DAMN IT!!!
	static char fp[MAX_PATH_CHARS];
	strncpy(fp, filepath.data(), filepath.size());
	fp[filepath.size()] = '\0';
	// RTLD_GLOBAL is required for allowing unique type_id<>() across shared library
	// boundaries; see the following
	// https://cpptruths.blogspot.com/2018/11/non-colliding-efficient.html (section:
	// Dynamically Loaded Libraries) https://linux.die.net/man/3/dlopen (section:
	// RTLD_GLOBAL) RTLD_NOW is simply used to ensure everything is resolved at dlopen,
	// therefore, showing the internal error if not resolved (since dlopen will return
	// NULL then) this ensures proper error output and exit instead of segfaulting or
	// something
	void *hndl = dlopen(fp, RTLD_NOW | RTLD_GLOBAL);
	if(hndl == nullptr) {
		err::out({"dyn lib failed to open ", filepath, ": ", dlerror()});
		return nullptr;
	}
	handles[filepath] = hndl;
	return hndl;
}
void DynLib::unload(StringRef filepath)
{
	auto handle = handles.find(filepath);
	if(handle == handles.end()) return;
	dlclose(handle->second);
	handles.erase(handle);
}
void *DynLib::get(StringRef filepath, const char *sym)
{
	auto handle = handles.find(filepath);
	if(handle == handles.end()) return nullptr;
	return dlsym(handle->second, sym);
}

} // namespace fer