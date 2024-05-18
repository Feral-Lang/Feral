#include "std/StructType.hpp"

#include <algorithm>

namespace fer
{

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// VarStructDef ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarStructDef::VarStructDef(const ModuleLoc *loc, size_t attrscount, size_t id)
	: Var(loc, true, true), attrorder(attrscount, ""), id(id)
{
	attrs.reserve(attrscount);
}

VarStructDef::~VarStructDef()
{
	for(auto &attr : attrs) {
		decref(attr.second);
	}
}

Var *VarStructDef::copy(const ModuleLoc *loc)
{
	VarStructDef *res = new VarStructDef(loc, attrs.size(), id);
	std::unordered_map<std::string, Var *> attrs;
	for(auto &attr : attrs) {
		res->attrs.insert({attr.first, attr.second->copy(loc)});
	}
	res->attrorder = attrorder;
	return res;
}

void VarStructDef::set(Var *from)
{
	VarStructDef *st = as<VarStructDef>(from);
	for(auto &attr : attrs) {
		decref(attr.second);
	}
	for(auto &attr : st->attrs) {
		incref(attr.second);
		attrs[attr.first] = attr.second;
	}
	attrorder.assign(st->attrorder.begin(), st->attrorder.end());
	id = st->id;
}

Var *VarStructDef::call(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
			const StringMap<AssnArgData> &assn_args)
{
	for(auto &aa : assn_args) {
		if(std::find(attrorder.begin(), attrorder.end(), aa.first) == attrorder.end()) {
			vm.fail(aa.second.val->getLoc(), "no attribute named '", aa.first,
				"' in the structure definition");
			return nullptr;
		}
	}

	VarStruct *res = vm.makeVar<VarStruct>(loc, this, attrs.size(), id);

	auto it = attrorder.begin();
	for(auto argit = args.begin() + 1; argit != args.end(); ++argit) {
		auto &arg = *argit;
		if(it == attrorder.end()) {
			vm.fail(arg->getLoc(),
				"provided more arguments than existing in structure definition");
			goto fail;
		}
		auto aloc = attrs.find(*it);
		if(aloc->second->getType() != arg->getType()) {
			vm.fail(arg->getLoc(), "expected type: ", vm.getTypeName(aloc->second),
				", found: ", vm.getTypeName(arg));
			goto fail;
		}
		res->setAttr(*it, arg->copy(loc), false);
		++it;
	}

	for(auto &aa : assn_args) {
		auto aloc = attrs.find(aa.first);
		if(aloc == attrs.end()) {
			vm.fail(aa.second.val->getLoc(), "attribute '", aa.first,
				"' does not exist in this structure");
			goto fail;
		}
		if(aloc->second->getType() != aa.second.val->getType()) {
			vm.fail(aa.second.val->getLoc(),
				"expected type: ", vm.getTypeName(aloc->second),
				", found: ", vm.getTypeName(aa.second.val));
			goto fail;
		}
		res->setAttr(aa.first, aa.second.val->copy(loc), false);
	}

	while(it < attrorder.end()) {
		if(!res->existsAttr(*it)) res->setAttr(*it, attrs[*it]->copy(loc), false);
		++it;
	}

	return res;
fail:
	vm.unmakeVar(res);
	return nullptr;
}

void VarStructDef::setAttr(StringRef name, Var *val, bool iref)
{
	auto loc = attrs.find(name);
	if(loc != attrs.end()) {
		decref(loc->second);
	}
	if(iref) incref(val);
	attrs.insert_or_assign(String(name), val);
}

Var *VarStructDef::getAttr(StringRef name)
{
	auto loc = attrs.find(name);
	if(loc == attrs.end()) return nullptr;
	return loc->second;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VarStruct ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarStruct::VarStruct(const ModuleLoc *loc, VarStructDef *base, size_t attrscount, size_t id)
	: Var(loc, false, true), base(base), id(id)
{
	if(base) incref(base);
	attrs.reserve(attrscount);
}

VarStruct::~VarStruct()
{
	for(auto &attr : attrs) {
		decref(attr.second);
	}
	if(base) decref(base);
}

size_t VarStruct::getTypeFnID() { return id; }

Var *VarStruct::copy(const ModuleLoc *loc)
{
	VarStruct *res = new VarStruct(loc, base, attrs.size(), id);
	for(auto &attr : attrs) {
		res->setAttr(attr.first, attr.second->copy(loc), false);
	}
	return res;
}

void VarStruct::set(Var *from)
{
	VarStruct *st = as<VarStruct>(from);

	for(auto &attr : attrs) {
		decref(attr.second);
	}
	for(auto &attr : st->attrs) {
		incref(attr.second);
		attrs[attr.first] = attr.second;
	}
	if(base) decref(base);
	if(st->base) incref(st->base);
	base = st->base;
	id   = st->id;
}

void VarStruct::setAttr(StringRef name, Var *val, bool iref)
{
	auto loc = attrs.find(name);
	if(loc != attrs.end()) {
		decref(loc->second);
	}
	if(iref) incref(val);
	attrs.insert_or_assign(String(name), val);
}

Var *VarStruct::getAttr(StringRef name)
{
	auto loc = attrs.find(name);
	if(loc == attrs.end()) return base ? base->getAttr(name) : nullptr;
	return loc->second;
}

} // namespace fer