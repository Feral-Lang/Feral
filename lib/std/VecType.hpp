#pragma once

#include "VM/Interpreter.hpp"

using namespace fer;

class VarVecIterator : public Var
{
	VarVec *vec;
	size_t curr;

public:
	VarVecIterator(const ModuleLoc *loc, VarVec *vec);
	~VarVecIterator();

	Var *copy(const ModuleLoc *loc);
	void set(Var *from);

	bool next(Var *&val);
};