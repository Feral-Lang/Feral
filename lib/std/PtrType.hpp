#pragma once

#include "VM/Interpreter.hpp"

using namespace fer;

class VarPtr : public Var
{
	Var *val;

public:
	VarPtr(const ModuleLoc *loc, Var *val);
	~VarPtr();

	Var *copy(const ModuleLoc *loc);
	void set(Var *from);

	void update(Var *with);

	inline Var *get() { return val; }
};