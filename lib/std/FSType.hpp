#pragma once

#include "FS.hpp"
#include "VM/Interpreter.hpp"

using namespace fer;

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// VarFile ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

class VarFile : public Var
{
	FILE *file;
	String mode;
	bool owner;

public:
	VarFile(const ModuleLoc *loc, FILE *const file, const String &mode,
		const bool owner = true);
	~VarFile();

	Var *copy(const ModuleLoc *loc);
	void set(Var *from);

	inline void setMode(StringRef newmode) { mode = newmode; }
	inline void setOwner(bool isowner) { owner = isowner; }

	inline FILE *&getFile() { return file; }
	inline StringRef getMode() { return mode; }
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