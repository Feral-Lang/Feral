#include "std/FSType.hpp"

#include "FS.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// VarFile ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarFile::VarFile(const ModuleLoc *loc, FILE *const file, const String &mode, const bool owner)
	: Var(loc, false, false), file(file), mode(mode), owner(owner)
{}
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

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VarFileIterator //////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarFileIterator::VarFileIterator(const ModuleLoc *loc, VarFile *file)
	: Var(loc, false, false), file(file)
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
	if(!val) return false;
	char *lineptr	= NULL;
	size_t len	= 0;
	String &valdata = val->get();
	if(getline(&lineptr, &len, file->getFile()) != -1) {
		valdata.clear();
		valdata = lineptr;
		free(lineptr);
		while(!valdata.empty() && valdata.back() == '\n') valdata.pop_back();
		while(!valdata.empty() && valdata.back() == '\r') valdata.pop_back();
		return true;
	}
	if(lineptr) free(lineptr);
	return false;
}