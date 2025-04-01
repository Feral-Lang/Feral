#pragma once

#include "VM/VarTypes.hpp"

namespace fer
{

class VarPtr : public Var
{
	Var *val;

	void onCreate(InterpreterState &vms) override;
	void onDestroy(InterpreterState &vms) override;
	Var *onCopy(InterpreterState &vms, ModuleLoc loc) override;
	void onSet(InterpreterState &vms, Var *from) override;

public:
	VarPtr(ModuleLoc loc, Var *val);

	void setVal(InterpreterState &vms, Var *newval);

	inline Var *getVal() { return val; }
};

} // namespace fer