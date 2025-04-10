#pragma once

#include "VM/VarTypes.hpp"

namespace fer
{

class VarPtr : public Var
{
	Var *val;

	void onCreate(MemoryManager &mem) override;
	void onDestroy(MemoryManager &mem) override;
	Var *onCopy(MemoryManager &mem, ModuleLoc loc) override;
	void onSet(MemoryManager &mem, Var *from) override;

public:
	VarPtr(ModuleLoc loc, Var *val);

	void setVal(MemoryManager &mem, Var *newval);

	inline Var *getVal() { return val; }
};

} // namespace fer