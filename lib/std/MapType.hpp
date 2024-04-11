#pragma once

#include "VM/Interpreter.hpp"

namespace fer
{

class VarMapIterator : public Var
{
	VarMap *map;
	StringMap<Var *>::iterator curr;

public:
	VarMapIterator(const ModuleLoc *loc, VarMap *map);
	~VarMapIterator();

	Var *copy(const ModuleLoc *loc);
	void set(Var *from);

	bool next(Var *&val, Interpreter &vm, const ModuleLoc *loc);
};

} // namespace fer