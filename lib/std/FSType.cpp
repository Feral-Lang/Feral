#include "std/FSType.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// VarFile ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarFile::VarFile(const ModuleLoc *loc, FILE *const file, const char *mode, const bool owner)
	: Var(loc, typeID<VarFile>(), false, false), file(file), owner(owner)
{
	strcpy(this->mode, mode);
}
VarFile::~VarFile()
{
	if(owner && file) fclose(file);
}

Var *VarFile::copy(const ModuleLoc *loc) { return new VarFile(loc, file, mode, false); }

void VarFile::set(Var *from)
{
	if(owner) fclose(file);
	owner = false;
	file  = as<VarFile>(from)->file;
}

void VarFile::setMode(StringRef newmode)
{
	strncpy(mode, newmode.data(), newmode.size());
	mode[newmode.size()] = '\0';
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VarFileIterator //////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarFileIterator::VarFileIterator(const ModuleLoc *loc, VarFile *file)
	: Var(loc, typeID<VarFileIterator>(), false, false), file(file)
{
	incref(file);
}
VarFileIterator::~VarFileIterator() { decref(file); }

Var *VarFileIterator::copy(const ModuleLoc *loc) { return new VarFileIterator(loc, file); }
void VarFileIterator::set(Var *from)
{
	decref(file);
	file = as<VarFileIterator>(from)->file;
	incref(file);
}

bool VarFileIterator::next(VarStr *&val)
{
	if(!val || !val->is<VarStr>()) return false;
	char *lineptr = NULL;
	size_t len    = 0;
	ssize_t read  = 0;
	if((read = getline(&lineptr, &len, file->getFile())) != -1) {
		val->get().clear();
		val->get() = lineptr;
		free(lineptr);
		while(!val->get().empty() && val->get().back() == '\n') val->get().pop_back();
		while(!val->get().empty() && val->get().back() == '\r') val->get().pop_back();
		return true;
	}
	if(lineptr) free(lineptr);
	return false;
}