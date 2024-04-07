#include "std/VecType.hpp"

VarVecIterator::VarVecIterator(const ModuleLoc *loc, VarVec *vec)
	: Var(loc, false, false), vec(vec), curr(0)
{
	incref(vec);
}
VarVecIterator::~VarVecIterator() { decref(vec); }

Var *VarVecIterator::copy(const ModuleLoc *loc) { return new VarVecIterator(loc, vec); }
void VarVecIterator::set(Var *from)
{
	VarVecIterator *f = as<VarVecIterator>(from);
	decref(vec);
	incref(f->vec);
	vec  = f->vec;
	curr = f->curr;
}

bool VarVecIterator::next(Var *&val)
{
	if(curr >= vec->get().size()) return false;
	val = vec->get()[curr++];
	return true;
}