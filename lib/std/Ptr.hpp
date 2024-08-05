#pragma once

#include "VM/VarTypes.hpp"

namespace fer
{

class VarPtr : public Var
{
	Var *val;

	void onCreate(Interpreter &vm) override;
	void onDestroy(Interpreter &vm) override;
	Var *onCopy(Interpreter &vm, const ModuleLoc *loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	VarPtr(const ModuleLoc *loc, Var *val);

	void setVal(Interpreter &vm, Var *newval);

	inline Var *getVal() { return val; }
};

} // namespace fer