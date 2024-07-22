#pragma once

#include "VM/Interpreter.hpp"

namespace fer
{

class VarPtr : public Var
{
	Var *val;

	Var *copyImpl(const ModuleLoc *loc) override;

public:
	VarPtr(const ModuleLoc *loc, Var *val);
	~VarPtr();

	void set(Var *from);

	void update(Var *with);

	inline Var *get() { return val; }
};

} // namespace fer