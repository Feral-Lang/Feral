#include "PtrType.hpp"

namespace fer
{

VarPtr::VarPtr(const ModuleLoc *loc, Var *val) : Var(loc, false, false), val(val) { incref(val); }
VarPtr::~VarPtr() { decref(val); }

Var *VarPtr::copyImpl(const ModuleLoc *loc) { return new VarPtr(loc, val); }
void VarPtr::set(Var *from)
{
	decref(val);
	val = as<VarPtr>(from)->val;
	incref(val);
}

void VarPtr::update(Var *with)
{
	decref(val);
	val = with;
	incref(val);
}

} // namespace fer