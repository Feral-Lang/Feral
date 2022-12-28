#pragma once

#include "VM/Interpreter.hpp"

using namespace fer;

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// VarFile ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

class VarFile : public Var
{
	FILE *file;
	char mode[10];
	bool owner;

public:
	VarFile(const ModuleLoc *loc, FILE *const file, const char *mode, const bool owner = true);
	~VarFile();

	Var *copy(const ModuleLoc *loc);
	void set(Var *from);

	void setMode(StringRef newmode);

	inline void setMode(const char *newmode) { strcpy(mode, newmode); }
	inline void setMode(const String &newmode) { strcpy(mode, newmode.c_str()); }

	inline void setOwner(bool isowner) { owner = isowner; }

	inline FILE *&getFile() { return file; }
	inline const char *getMode() { return mode; }
	inline bool isOwner() { return owner; }
};

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VarFileIterator //////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

class VarFileIterator : public Var
{
	VarFile *file;

public:
	VarFileIterator(const ModuleLoc *loc, VarFile *file);
	~VarFileIterator();

	Var *copy(const ModuleLoc *loc);
	void set(Var *from);

	bool next(VarStr *&val);
};