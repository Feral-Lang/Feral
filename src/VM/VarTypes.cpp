#include "VM/VarTypes.hpp"

#include "VM/InterpreterThread.hpp"

#if defined(FER_OS_WINDOWS)
#include "FS.hpp" // required for getline()
#endif

namespace fer
{

static size_t genStructEnumID()
{
	static size_t id = -1;
	return id--;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// Var ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var::Var(ModuleLoc loc, bool callable, bool attr_based) : loc(loc), ref(1), info(0)
{
	if(callable) info |= (size_t)VarInfo::CALLABLE;
	if(attr_based) info |= (size_t)VarInfo::ATTR_BASED;
}
Var::~Var() {}

void Var::create(InterpreterState &vms) { onCreate(vms); }
void Var::destroy(InterpreterState &vms) { onDestroy(vms); }
Var *Var::copy(InterpreterState &vms, ModuleLoc loc) { return onCopy(vms, loc); }
void Var::set(InterpreterState &vms, Var *from) { onSet(vms, from); }

void Var::onCreate(InterpreterState &vms) {}
void Var::onDestroy(InterpreterState &vms) {}
Var *Var::onCopy(InterpreterState &vms, ModuleLoc loc) { return vms.incVarRef(this); }
void Var::onSet(InterpreterState &vms, Var *from) {}

size_t Var::getTypeFnID() { return getType(); }

Var *Var::call(InterpreterThread &vm, ModuleLoc loc, Span<Var *> args,
	       const StringMap<AssnArgData> &assn_args)
{
	return nullptr;
}
void Var::setAttr(InterpreterState &vms, StringRef name, Var *val, bool iref) {}
bool Var::existsAttr(StringRef name) { return false; }
Var *Var::getAttr(StringRef name) { return nullptr; }

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VarAll ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarAll::VarAll(ModuleLoc loc) : Var(loc, false, false) {}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VarNil ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarNil::VarNil(ModuleLoc loc) : Var(loc, false, false) {}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// VarTypeID //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarTypeID::VarTypeID(ModuleLoc loc, size_t val) : Var(loc, false, false), val(val) {}
Var *VarTypeID::onCopy(InterpreterState &vms, ModuleLoc loc)
{
	return vms.makeVarWithRef<VarTypeID>(loc, val);
}
void VarTypeID::onSet(InterpreterState &vms, Var *from) { val = as<VarTypeID>(from)->getVal(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarBool ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarBool::VarBool(ModuleLoc loc, bool val) : Var(loc, false, false), val(val) {}
Var *VarBool::onCopy(InterpreterState &vms, ModuleLoc loc)
{
	return vms.makeVarWithRef<VarBool>(loc, val);
}
void VarBool::onSet(InterpreterState &vms, Var *from) { val = as<VarBool>(from)->getVal(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarInt ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarInt::VarInt(ModuleLoc loc, int64_t _val) : Var(loc, false, false), val(_val) {}
VarInt::VarInt(ModuleLoc loc, const char *_val) : Var(loc, false, false), val(std::stoll(_val)) {}
Var *VarInt::onCopy(InterpreterState &vms, ModuleLoc loc)
{
	return vms.makeVarWithRef<VarInt>(loc, val);
}
void VarInt::onSet(InterpreterState &vms, Var *from) { val = as<VarInt>(from)->getVal(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// VarIntIterator ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarIntIterator::VarIntIterator(ModuleLoc loc)
	: Var(loc, false, false), started(false), reversed(false), begin(0), end(0), step(0),
	  curr(0)
{}
VarIntIterator::VarIntIterator(ModuleLoc loc, int64_t _begin, int64_t _end, int64_t _step)
	: Var(loc, false, false), started(false), reversed(_step < 0), begin(_begin), end(_end),
	  step(_step), curr(_begin)
{}

Var *VarIntIterator::onCopy(InterpreterState &vms, ModuleLoc loc)
{
	return vms.makeVarWithRef<VarIntIterator>(loc, begin, end, step);
}
void VarIntIterator::onSet(InterpreterState &vms, Var *from)
{
	VarIntIterator *f = as<VarIntIterator>(from);

	begin	 = f->begin;
	end	 = f->end;
	step	 = f->step;
	curr	 = f->curr;
	started	 = f->started;
	reversed = f->reversed;
}

bool VarIntIterator::next(int64_t &val)
{
	if(reversed) {
		if(curr <= end) return false;
	} else {
		if(curr >= end) return false;
	}
	if(!started) {
		val	= curr;
		started = true;
		return true;
	}
	int64_t tmp = curr + step;
	if(reversed) {
		if(tmp <= end) return false;
	} else {
		if(tmp >= end) return false;
	}
	curr = tmp;
	val  = curr;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarFlt ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarFlt::VarFlt(ModuleLoc loc, long double _val) : Var(loc, false, false), val(_val) {}
VarFlt::VarFlt(ModuleLoc loc, const char *_val) : Var(loc, false, false), val(std::stold(_val)) {}
Var *VarFlt::onCopy(InterpreterState &vms, ModuleLoc loc)
{
	return vms.makeVarWithRef<VarFlt>(loc, val);
}
void VarFlt::onSet(InterpreterState &vms, Var *from) { val = as<VarFlt>(from)->getVal(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarStr ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarStr::VarStr(ModuleLoc loc, char val) : Var(loc, false, false), val(1, val) {}
VarStr::VarStr(ModuleLoc loc, String &&val) : Var(loc, false, false), val(std::move(val)) {}
VarStr::VarStr(ModuleLoc loc, StringRef val) : Var(loc, false, false), val(val) {}
VarStr::VarStr(ModuleLoc loc, const char *val) : Var(loc, false, false), val(val) {}
VarStr::VarStr(ModuleLoc loc, InitList<StringRef> _val) : Var(loc, false, false)
{
	for(auto &e : _val) val += e;
}
VarStr::VarStr(ModuleLoc loc, const char *val, size_t count)
	: Var(loc, false, false), val(val, count)
{}
Var *VarStr::onCopy(InterpreterState &vms, ModuleLoc loc)
{
	return vms.makeVarWithRef<VarStr>(loc, val);
}
void VarStr::onSet(InterpreterState &vms, Var *from) { val = as<VarStr>(from)->getVal(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarVec ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarVec::VarVec(ModuleLoc loc, size_t reservesz, bool asrefs)
	: Var(loc, false, false), asrefs(asrefs)
{
	val.reserve(reservesz);
}
VarVec::VarVec(ModuleLoc loc, Vector<Var *> &&val, bool asrefs)
	: Var(loc, false, false), val(std::move(val)), asrefs(asrefs)
{}
void VarVec::onDestroy(InterpreterState &vms) { clear(vms); }
Var *VarVec::onCopy(InterpreterState &vms, ModuleLoc loc)
{
	VarVec *tmp = vms.makeVarWithRef<VarVec>(loc, val.size(), asrefs);
	tmp->setVal(vms, val);
	return tmp;
}
void VarVec::onSet(InterpreterState &vms, Var *from) { setVal(vms, as<VarVec>(from)->getVal()); }
void VarVec::setVal(InterpreterState &vms, Span<Var *> newval)
{
	clear(vms);
	if(asrefs) {
		for(auto &v : newval) {
			vms.incVarRef(v);
		}
		val.assign(newval.begin(), newval.end());
	} else {
		for(auto &v : newval) val.push_back(vms.copyVar(getLoc(), v));
	}
}
void VarVec::clear(InterpreterState &vms)
{
	for(auto &v : val) vms.decVarRef(v);
	val.clear();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// VarVecIterator ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarVecIterator::VarVecIterator(ModuleLoc loc, VarVec *vec)
	: Var(loc, false, false), vec(vec), curr(0)
{}
void VarVecIterator::onCreate(InterpreterState &vms) { vms.incVarRef(vec); }
void VarVecIterator::onDestroy(InterpreterState &vms) { vms.decVarRef(vec); }
Var *VarVecIterator::onCopy(InterpreterState &vms, ModuleLoc loc)
{
	return vms.makeVarWithRef<VarVecIterator>(loc, vec);
}
void VarVecIterator::onSet(InterpreterState &vms, Var *from)
{
	VarVecIterator *f = as<VarVecIterator>(from);
	vms.decVarRef(vec);
	vms.incVarRef(f->vec);
	vec  = f->vec;
	curr = f->curr;
}

bool VarVecIterator::next(Var *&val)
{
	if(curr >= vec->getVal().size()) return false;
	val = vec->getVal()[curr++];
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarMap ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarMap::VarMap(ModuleLoc loc, size_t reservesz, bool asrefs) : Var(loc, false, true), asrefs(asrefs)
{
	val.reserve(reservesz);
}
VarMap::VarMap(ModuleLoc loc, StringMap<Var *> &&val, bool asrefs)
	: Var(loc, false, true), val(std::move(val)), asrefs(asrefs)
{}
void VarMap::onDestroy(InterpreterState &vms) { clear(vms); }
Var *VarMap::onCopy(InterpreterState &vms, ModuleLoc loc)
{
	VarMap *tmp = vms.makeVarWithRef<VarMap>(loc, val.size(), asrefs);
	tmp->setVal(vms, val);
	return tmp;
}
void VarMap::onSet(InterpreterState &vms, Var *from) { setVal(vms, as<VarMap>(from)->getVal()); }
void VarMap::setVal(InterpreterState &vms, const StringMap<Var *> &newval)
{
	clear(vms);
	if(asrefs) {
		for(auto &v : newval) {
			vms.incVarRef(v.second);
		}
		val.insert(newval.begin(), newval.end());
	} else {
		for(auto &v : newval) val.insert({v.first, vms.copyVar(getLoc(), v.second)});
	}
}
void VarMap::clear(InterpreterState &vms)
{
	for(auto &v : val) vms.decVarRef(v.second);
	val.clear();
}
void VarMap::setAttr(InterpreterState &vms, StringRef name, Var *val, bool iref)
{
	auto loc = this->val.find(name);
	if(iref) vms.incVarRef(val);
	if(loc == this->val.end()) {
		this->val.insert({String(name), val});
		return;
	}
	vms.decVarRef(loc->second);
	loc->second = val;
}
bool VarMap::existsAttr(StringRef name) { return this->val.find(name) != this->val.end(); }
Var *VarMap::getAttr(StringRef name)
{
	auto loc = this->val.find(name);
	if(loc == this->val.end()) return nullptr;
	return loc->second;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// VarMapIterator ///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarMapIterator::VarMapIterator(ModuleLoc loc, VarMap *map)
	: Var(loc, false, false), map(map), curr(map->getVal().begin())
{}
void VarMapIterator::onCreate(InterpreterState &vms) { vms.incVarRef(map); }
void VarMapIterator::onDestroy(InterpreterState &vms) { vms.decVarRef(map); }
Var *VarMapIterator::onCopy(InterpreterState &vms, ModuleLoc loc)
{
	return vms.makeVarWithRef<VarMapIterator>(loc, map);
}
void VarMapIterator::onSet(InterpreterState &vms, Var *from)
{
	VarMapIterator *f = as<VarMapIterator>(from);
	vms.decVarRef(map);
	vms.incVarRef(f->map);
	map  = f->map;
	curr = f->curr;
}

bool VarMapIterator::next(InterpreterState &vms, ModuleLoc loc, Var *&val)
{
	if(curr == map->getVal().end()) return false;
	StringMap<Var *> attrs;
	val = vms.makeVar<VarStruct>(loc, nullptr, 2, typeID<VarMapIterator>());
	as<VarStruct>(val)->setAttr(vms, "0", vms.makeVarWithRef<VarStr>(loc, curr->first), false);
	as<VarStruct>(val)->setAttr(vms, "1", vms.incVarRef(curr->second), false);
	++curr;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarFn /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarFn::VarFn(ModuleLoc loc, ModuleId moduleId, const String &kw_arg, const String &var_arg,
	     size_t paramcount, size_t assn_params_count, FnBody body, bool is_native)
	: Var(loc, true, false), moduleId(moduleId), kw_arg(kw_arg), var_arg(var_arg), body(body),
	  is_native(is_native)
{
	params.reserve(paramcount);
	assn_params.reserve(assn_params_count);
}
void VarFn::onDestroy(InterpreterState &vms)
{
	for(auto &aa : assn_params) vms.decVarRef(aa.second);
}
Var *VarFn::onCopy(InterpreterState &vms, ModuleLoc loc)
{
	VarFn *tmp = vms.makeVarWithRef<VarFn>(loc, moduleId, kw_arg, var_arg, params.size(),
					       assn_params.size(), body, is_native);
	tmp->setParams(params);
	for(auto &aa : assn_params) vms.incVarRef(aa.second);
	tmp->setAssnParams(assn_params);
	return tmp;
}
void VarFn::onSet(InterpreterState &vms, Var *from)
{
	VarFn *tmp = as<VarFn>(from);

	moduleId = tmp->moduleId;
	kw_arg	 = tmp->kw_arg;
	var_arg	 = tmp->var_arg;
	// I assume copy assignment operator of vector & map do not cause unnecessary reallocations
	params = tmp->params;
	for(auto &aa : assn_params) vms.decVarRef(aa.second);
	for(auto &aa : tmp->assn_params) vms.incVarRef(aa.second);
	assn_params = tmp->assn_params;
	body	    = tmp->body;
	is_native   = tmp->is_native;
}
Var *VarFn::call(InterpreterThread &vm, ModuleLoc loc, Span<Var *> args,
		 const StringMap<AssnArgData> &assn_args)
{
	// -1 for self
	if(args.size() - 1 < params.size() - assn_params.size() ||
	   (args.size() - 1 > params.size() && var_arg.empty()))
	{
		vm.fail(loc, "arg count required: ", params.size(),
			" (without default args: ", params.size() - assn_args.size(),
			"); received: ", args.size() - 1);
		return nullptr;
	}
	if(isNative()) {
		Var *res = body.native(vm, loc, args, assn_args);
		if(!res) return nullptr;
		vm.pushExecStack(res);
		return vm.getNil();
	}
	vm.pushModule(moduleId);
	Vars *vars = vm.getCurrModule()->getVars();
	// take care of 'self' (always present - either data or nullptr)
	if(args[0] != nullptr) vars->stash("self", args[0]);
	// default arguments
	Set<StringRef> found_args;
	size_t i = 1;
	for(auto &a : params) {
		if(i == args.size()) break;
		vars->stash(a, args[i++]);
		found_args.insert(a);
	}
	// add all default args which have not been overwritten by args
	for(auto &aa : assn_params) {
		if(found_args.find(aa.first) != found_args.end()) continue;
		Var *cpy = vm.copyVar(loc, aa.second);
		vars->stash(aa.first, cpy, false); // copy will make sure there is ref = 1 already
	}
	if(!var_arg.empty()) {
		VarVec *v = vm.makeVarWithRef<VarVec>(loc, args.size(), false);
		while(i < args.size()) {
			vm.incVarRef(args[i]);
			v->push(args[i]);
			++i;
		}
		vars->stash(var_arg, v, false);
	}
	if(!kw_arg.empty()) {
		VarMap *m = vm.makeVarWithRef<VarMap>(loc, assn_args.size(), false);
		m->initializePos(assn_args.size());
		for(auto &a : assn_args) {
			vm.incVarRef(a.second.val);
			m->insert(a.first, a.second.val);
			m->setPos(a.second.pos, a.first);
		}
		vars->stash(kw_arg, m, false);
	}
	if(vm.execute(true, false, body.feral.begin, body.feral.end) != 0 && !vm.isExitCalled()) {
		vars->unstash();
		vm.popModule();
		return nullptr;
	}
	vm.popModule();
	return vm.getNil();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// VarModule ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarModule::VarModule(ModuleLoc loc, StringRef path, Bytecode &&bc, ModuleId moduleId, Vars *vars)
	: Var(loc, false, true), path(path), bc(std::move(bc)), moduleId(moduleId), vars(vars),
	  ownsVars(vars == nullptr)
{}
void VarModule::onCreate(InterpreterState &vms)
{
	if(vars == nullptr) this->vars = new Vars(vms);
}
void VarModule::onDestroy(InterpreterState &vms)
{
	if(vars && ownsVars) delete vars;
}
void VarModule::setAttr(InterpreterState &vms, StringRef name, Var *val, bool iref)
{
	vars->add(name, val, iref);
}
bool VarModule::existsAttr(StringRef name) { return vars->exists(name); }
Var *VarModule::getAttr(StringRef name) { return vars->get(name); }

void VarModule::addNativeFn(InterpreterThread &vm, StringRef name, NativeFn body, size_t args,
			    bool is_va)
{
	VarFn *res = vm.makeVarWithRef<VarFn>(getLoc(), moduleId, "", is_va ? "." : "", args, 0,
					      FnBody{.native = body}, true);
	for(size_t i = 0; i < args; ++i) res->pushParam("");
	vars->add(name, res, false);
}
void VarModule::addNativeVar(StringRef name, Var *val, bool iref, bool module_level)
{
	if(module_level) vars->addm(name, val, iref);
	else vars->add(name, val, iref);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// VarStructDef ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarStructDef::VarStructDef(ModuleLoc loc, size_t attrscount)
	: Var(loc, true, true), attrorder(attrscount, ""), id(genStructEnumID())
{
	attrs.reserve(attrscount);
}
VarStructDef::VarStructDef(ModuleLoc loc, size_t attrscount, size_t id)
	: Var(loc, true, true), attrorder(attrscount, ""), id(id)
{
	attrs.reserve(attrscount);
}
void VarStructDef::onDestroy(InterpreterState &vms)
{
	for(auto &attr : attrs) {
		vms.decVarRef(attr.second);
	}
}

Var *VarStructDef::onCopy(InterpreterState &vms, ModuleLoc loc)
{
	VarStructDef *res = vms.makeVarWithRef<VarStructDef>(loc, attrs.size(), id);
	std::unordered_map<std::string, Var *> attrs;
	for(auto &attr : attrs) {
		res->attrs.insert({attr.first, vms.copyVar(loc, attr.second)});
	}
	res->attrorder = attrorder;
	return res;
}

void VarStructDef::onSet(InterpreterState &vms, Var *from)
{
	VarStructDef *st = as<VarStructDef>(from);
	for(auto &attr : attrs) {
		vms.decVarRef(attr.second);
	}
	for(auto &attr : st->attrs) {
		vms.incVarRef(attr.second);
		attrs[attr.first] = attr.second;
	}
	attrorder.assign(st->attrorder.begin(), st->attrorder.end());
	id = st->id;
}

Var *VarStructDef::call(InterpreterThread &vm, ModuleLoc loc, Span<Var *> args,
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

	InterpreterState &vms = vm.getGlobalState();
	auto it		      = attrorder.begin();
	for(auto argit = args.begin() + 1; argit != args.end(); ++argit) {
		auto &arg = *argit;
		if(it == attrorder.end()) {
			vm.fail(arg->getLoc(),
				"provided more arguments than existing in structure definition");
			goto fail;
		}
		res->setAttr(vms, *it, vm.copyVar(loc, arg), false);
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
		res->setAttr(vms, aa.first, vm.copyVar(loc, aa.second.val), false);
	}

	while(it < attrorder.end()) {
		if(!res->existsAttr(*it))
			res->setAttr(vms, *it, vm.copyVar(loc, attrs[*it]), false);
		++it;
	}

	return res;
fail:
	vm.unmakeVar(res);
	return nullptr;
}

void VarStructDef::setAttr(InterpreterState &vms, StringRef name, Var *val, bool iref)
{
	auto loc = attrs.find(name);
	if(loc != attrs.end()) {
		vms.decVarRef(loc->second);
	}
	if(iref) vms.incVarRef(val);
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

VarStruct::VarStruct(ModuleLoc loc, VarStructDef *base, size_t attrscount)
	: Var(loc, false, true), base(base), id(genStructEnumID())
{
	attrs.reserve(attrscount);
}
VarStruct::VarStruct(ModuleLoc loc, VarStructDef *base, size_t attrscount, size_t id)
	: Var(loc, false, true), base(base), id(id)
{
	attrs.reserve(attrscount);
}
void VarStruct::onCreate(InterpreterState &vms)
{
	if(base) vms.incVarRef(base);
}
void VarStruct::onDestroy(InterpreterState &vms)
{
	for(auto &attr : attrs) {
		vms.decVarRef(attr.second);
	}
	if(base) vms.decVarRef(base);
}
Var *VarStruct::onCopy(InterpreterState &vms, ModuleLoc loc)
{
	VarStruct *res = vms.makeVarWithRef<VarStruct>(loc, base, attrs.size(), id);
	for(auto &attr : attrs) {
		res->setAttr(vms, attr.first, vms.copyVar(loc, attr.second), false);
	}
	return res;
}

void VarStruct::onSet(InterpreterState &vms, Var *from)
{
	VarStruct *st = as<VarStruct>(from);

	for(auto &attr : attrs) {
		vms.decVarRef(attr.second);
	}
	for(auto &attr : st->attrs) {
		vms.incVarRef(attr.second);
		attrs[attr.first] = attr.second;
	}
	if(base) vms.decVarRef(base);
	if(st->base) vms.incVarRef(st->base);
	base = st->base;
	id   = st->id;
}

void VarStruct::setAttr(InterpreterState &vms, StringRef name, Var *val, bool iref)
{
	auto loc = attrs.find(name);
	if(loc != attrs.end()) {
		vms.decVarRef(loc->second);
	}
	if(iref) vms.incVarRef(val);
	attrs.insert_or_assign(String(name), val);
}

Var *VarStruct::getAttr(StringRef name)
{
	auto loc = attrs.find(name);
	if(loc == attrs.end()) return base ? base->getAttr(name) : nullptr;
	return loc->second;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// VarFile ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarFile::VarFile(ModuleLoc loc, FILE *const file, const String &mode, const bool owner)
	: Var(loc, false, false), file(file), mode(mode), owner(owner)
{}
void VarFile::onDestroy(InterpreterState &vms)
{
	if(owner && file) fclose(file);
}

Var *VarFile::onCopy(InterpreterState &vms, ModuleLoc loc)
{
	return vms.makeVarWithRef<VarFile>(loc, file, mode, false);
}

void VarFile::onSet(InterpreterState &vms, Var *from)
{
	if(owner) fclose(file);
	owner = false;
	file  = as<VarFile>(from)->file;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VarFileIterator //////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarFileIterator::VarFileIterator(ModuleLoc loc, VarFile *file) : Var(loc, false, false), file(file)
{}
void VarFileIterator::onCreate(InterpreterState &vms) { vms.incVarRef(file); }
void VarFileIterator::onDestroy(InterpreterState &vms) { vms.decVarRef(file); }

Var *VarFileIterator::onCopy(InterpreterState &vms, ModuleLoc loc)
{
	return vms.makeVarWithRef<VarFileIterator>(loc, file);
}
void VarFileIterator::onSet(InterpreterState &vms, Var *from)
{
	vms.decVarRef(file);
	file = as<VarFileIterator>(from)->file;
	vms.incVarRef(file);
}

bool VarFileIterator::next(VarStr *&val)
{
	if(!val) return false;
	char *lineptr	= NULL;
	size_t len	= 0;
	String &valdata = val->getVal();
	if(getline(&lineptr, &len, file->getFile()) != -1) {
		valdata.clear();
		valdata = lineptr;
		free(lineptr);
		while(!valdata.empty() && valdata.back() == '\n') valdata.pop_back();
		while(!valdata.empty() && valdata.back() == '\r') valdata.pop_back();
		return true;
	}
	if(lineptr) free(lineptr);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// VarByteBuffer ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarBytebuffer::VarBytebuffer(ModuleLoc loc, size_t bufsz, size_t buflen, char *buf)
	: Var(loc, false, false), buffer(nullptr), bufsz(bufsz), buflen(buflen)
{
	if(bufsz > 0) buffer = (char *)malloc(bufsz);
	if(buflen > 0) memcpy(buffer, buf, buflen);
}
VarBytebuffer::~VarBytebuffer()
{
	if(bufsz > 0) free(buffer);
}
Var *VarBytebuffer::onCopy(InterpreterState &vms, ModuleLoc loc)
{
	return vms.makeVarWithRef<VarBytebuffer>(loc, bufsz, buflen, buffer);
}
void VarBytebuffer::onSet(InterpreterState &vms, Var *from)
{
	VarBytebuffer *tmp = as<VarBytebuffer>(from);
	setData(tmp->buffer, tmp->buflen);
}
void VarBytebuffer::setData(char *newbuf, size_t newlen)
{
	if(newlen == 0) return;
	if(bufsz > 0 && newlen > bufsz) {
		buffer = (char *)realloc(buffer, newlen);
		bufsz  = newlen;
	} else if(bufsz == 0) {
		buffer = (char *)malloc(newlen);
		bufsz  = newlen;
	}
	memcpy(buffer, newbuf, newlen);
	buflen = newlen;
}

} // namespace fer