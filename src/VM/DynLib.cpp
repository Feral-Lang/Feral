#include "VM/DynLib.hpp"

#include "Error.hpp"

#if defined(CORE_OS_WINDOWS)
// Windows dlfcn equivalent functions.
// Thanks to this person's youth:
// https://stackoverflow.com/questions/53530566/loading-dll-in-windows-c-for-cross-platform-design
#include <Windows.h>

#include "FS.hpp"

#define RTLD_GLOBAL 0x100 /* do not hide entries in this module */
#define RTLD_LOCAL 0x000  /* hide entries in this module */

#define RTLD_LAZY 0x000 /* accept unresolved externs */
#define RTLD_NOW 0x001	/* abort if module has unresolved externs */

static struct
{
	long lasterror;
	const char *err_rutin;
} var = {0, NULL};

void *dlopen(const char *filename, int flags);
int dlclose(void *handle);
void *dlsym(void *handle, const char *name);
const char *dlerror();
#else
#include <dlfcn.h>
#endif

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

void *DynLib::load(const char *filepath)
{
	auto handle = handles.find(filepath);
	if(handle != handles.end()) return handle->second;

	// RTLD_GLOBAL is required for allowing unique typeId<>() across shared library
	// boundaries; see the following
	// https://cpptruths.blogspot.com/2018/11/non-colliding-efficient.html (section:
	// Dynamically Loaded Libraries) https://linux.die.net/man/3/dlopen (section:
	// RTLD_GLOBAL) RTLD_NOW is simply used to ensure everything is resolved at dlopen,
	// therefore, showing the internal error if not resolved (since dlopen will return
	// NULL then) this ensures proper error output and exit instead of segfaulting or
	// something
	void *hndl = dlopen(filepath, RTLD_NOW | RTLD_GLOBAL);
	if(hndl == nullptr) {
		err.fail({}, "dyn lib failed to open ", filepath, ": ", dlerror());
	} else {
		handles.insert({filepath, hndl});
	}
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

// Windows dlfcn equivalent function definitions
#if defined(CORE_OS_WINDOWS)
void *dlopen(const char *filename, int flags)
{
	// We need to set current working directory to the module's directory because
	// Windows can't find the dependent modules (assuming one is in the same directory)
	// otherwise.
	using namespace fer;
	String prevdir = fs::getCWD();
	fs::setCWD(String(fs::parentDir(filename)).c_str());
	HINSTANCE hInst = LoadLibraryA(filename);
	if(hInst == NULL) {
		var.lasterror = GetLastError();
		var.err_rutin = "dlopen";
	}
	fs::setCWD(prevdir.c_str());
	return hInst;
}

int dlclose(void *handle)
{
	int rc = 0;
	if(!FreeLibrary((HINSTANCE)handle)) {
		var.lasterror = GetLastError();
		var.err_rutin = "dlclose";
		rc	      = -1;
	}
	return rc;
}

void *dlsym(void *handle, const char *name)
{
	FARPROC fp = GetProcAddress((HINSTANCE)handle, name);
	if(!fp) {
		var.lasterror = GetLastError();
		var.err_rutin = "dlsym";
	}
	return (void *)(uintptr_t)fp;
}

const char *dlerror(void)
{
	if(!var.lasterror) return nullptr;
	void *msg = nullptr;
	FormatMessageA(
	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	NULL, var.lasterror, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char *)&msg, 0, NULL);
	static char errstr[1024];
	sprintf(errstr, "%s error #%ld: %s", var.err_rutin, var.lasterror, (char *)msg);
	return errstr;
}
#endif
