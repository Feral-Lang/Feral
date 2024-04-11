#include "std/MapType.hpp"

#include "std/StructType.hpp"

namespace fer
{

VarMapIterator::VarMapIterator(const ModuleLoc *loc, VarMap *map)
	: Var(loc, false, false), map(map), curr(map->get().begin())
{
	incref(map);
}
VarMapIterator::~VarMapIterator() { decref(map); }

Var *VarMapIterator::copy(const ModuleLoc *loc) { return new VarMapIterator(loc, map); }
void VarMapIterator::set(Var *from)
{
	VarMapIterator *f = as<VarMapIterator>(from);
	decref(map);
	incref(f->map);
	map  = f->map;
	curr = f->curr;
}

bool VarMapIterator::next(Var *&val, Interpreter &vm, const ModuleLoc *loc)
{
	if(curr == map->get().end()) return false;
	StringMap<Var *> attrs;
	incref(curr->second);
	val = vm.makeVar<VarStruct>(loc, nullptr, 2, typeID<VarMapIterator>());
	as<VarStruct>(val)->setAttr("0", vm.makeVarWithRef<VarStr>(loc, curr->first), false);
	as<VarStruct>(val)->setAttr("1", curr->second, false);
	++curr;
	return true;
}

} // namespace fer