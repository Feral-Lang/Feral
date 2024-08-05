#pragma once

#include "VM/VarTypes.hpp"

namespace fer
{

class VarPtr : public Var
{
	Var *val;

	Var *copyImpl(const ModuleLoc *loc) override;

public:
	VarPtr(const ModuleLoc *loc, Var *val);
	~VarPtr();

	void set(Var *from) override;

	void update(Var *with);

	inline Var *get() { return val; }
};

} // namespace fer