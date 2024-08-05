#include "VM/VarTypes.hpp"

#include "VM/Interpreter.hpp"

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

Var::Var(const ModuleLoc *loc, bool callable, bool attr_based) : loc(loc), ref(1), info(0)
{
	if(callable) info |= (size_t)VarInfo::CALLABLE;
	if(attr_based) info |= (size_t)VarInfo::ATTR_BASED;
}
Var::~Var() {}

void Var::create(Interpreter &vm) { onCreate(vm); }
void Var::destroy(Interpreter &vm) { onDestroy(vm); }
Var *Var::copy(Interpreter &vm, const ModuleLoc *loc) { return onCopy(vm, loc); }
void Var::set(Interpreter &vm, Var *from) { onSet(vm, from); }

void Var::onCreate(Interpreter &vm) {}
void Var::onDestroy(Interpreter &vm) {}
Var *Var::onCopy(Interpreter &vm, const ModuleLoc *loc) { return vm.incVarRef(this); }
void Var::onSet(Interpreter &vm, Var *from) {}

size_t Var::getTypeFnID() { return getType(); }

Var *Var::call(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const StringMap<AssnArgData> &assn_args)
{
	return nullptr;
}
void Var::setAttr(Interpreter &vm, StringRef name, Var *val, bool iref) {}
bool Var::existsAttr(StringRef name) { return false; }
Var *Var::getAttr(StringRef name) { return nullptr; }

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VarAll ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarAll::VarAll(const ModuleLoc *loc) : Var(loc, false, false) {}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VarNil ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarNil::VarNil(const ModuleLoc *loc) : Var(loc, false, false) {}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// VarTypeID //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarTypeID::VarTypeID(const ModuleLoc *loc, size_t val) : Var(loc, false, false), val(val) {}
Var *VarTypeID::onCopy(Interpreter &vm, const ModuleLoc *loc)
{
	return vm.makeVarWithRef<VarTypeID>(loc, val);
}
void VarTypeID::onSet(Interpreter &vm, Var *from) { val = as<VarTypeID>(from)->getVal(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarBool ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarBool::VarBool(const ModuleLoc *loc, bool val) : Var(loc, false, false), val(val) {}
Var *VarBool::onCopy(Interpreter &vm, const ModuleLoc *loc)
{
	return vm.makeVarWithRef<VarBool>(loc, val);
}
void VarBool::onSet(Interpreter &vm, Var *from) { val = as<VarBool>(from)->getVal(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarInt ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarInt::VarInt(const ModuleLoc *loc, int64_t _val) : Var(loc, false, false), val(_val) {}
VarInt::VarInt(const ModuleLoc *loc, const char *_val)
	: Var(loc, false, false), val(std::stoll(_val))
{}
Var *VarInt::onCopy(Interpreter &vm, const ModuleLoc *loc)
{
	return vm.makeVarWithRef<VarInt>(loc, val);
}
void VarInt::onSet(Interpreter &vm, Var *from) { val = as<VarInt>(from)->getVal(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// VarIntIterator ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarIntIterator::VarIntIterator(const ModuleLoc *loc)
	: Var(loc, false, false), started(false), reversed(false), begin(0), end(0), step(0),
	  curr(0)
{}
VarIntIterator::VarIntIterator(const ModuleLoc *loc, int64_t _begin, int64_t _end, int64_t _step)
	: Var(loc, false, false), started(false), reversed(_step < 0), begin(_begin), end(_end),
	  step(_step), curr(_begin)
{}

Var *VarIntIterator::onCopy(Interpreter &vm, const ModuleLoc *loc)
{
	return vm.makeVarWithRef<VarIntIterator>(loc, begin, end, step);
}
void VarIntIterator::onSet(Interpreter &vm, Var *from)
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

VarFlt::VarFlt(const ModuleLoc *loc, long double _val) : Var(loc, false, false), val(_val) {}
VarFlt::VarFlt(const ModuleLoc *loc, const char *_val)
	: Var(loc, false, false), val(std::stold(_val))
{}
Var *VarFlt::onCopy(Interpreter &vm, const ModuleLoc *loc)
{
	return vm.makeVarWithRef<VarFlt>(loc, val);
}
void VarFlt::onSet(Interpreter &vm, Var *from) { val = as<VarFlt>(from)->getVal(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarStr ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarStr::VarStr(const ModuleLoc *loc, char val) : Var(loc, false, false), val(1, val) {}
VarStr::VarStr(const ModuleLoc *loc, String &&val) : Var(loc, false, false), val(std::move(val)) {}
VarStr::VarStr(const ModuleLoc *loc, StringRef val) : Var(loc, false, false), val(val) {}
VarStr::VarStr(const ModuleLoc *loc, const char *val) : Var(loc, false, false), val(val) {}
VarStr::VarStr(const ModuleLoc *loc, InitList<StringRef> _val) : Var(loc, false, false)
{
	for(auto &e : _val) val += e;
}
VarStr::VarStr(const ModuleLoc *loc, const char *val, size_t count)
	: Var(loc, false, false), val(val, count)
{}
Var *VarStr::onCopy(Interpreter &vm, const ModuleLoc *loc)
{
	return vm.makeVarWithRef<VarStr>(loc, val);
}
void VarStr::onSet(Interpreter &vm, Var *from) { val = as<VarStr>(from)->getVal(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarVec ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarVec::VarVec(const ModuleLoc *loc, size_t reservesz, bool asrefs)
	: Var(loc, false, false), asrefs(asrefs)
{
	val.reserve(reservesz);
}
VarVec::VarVec(const ModuleLoc *loc, Vector<Var *> &&val, bool asrefs)
	: Var(loc, false, false), val(std::move(val)), asrefs(asrefs)
{}
void VarVec::onDestroy(Interpreter &vm)
{
	for(auto &v : val) vm.decVarRef(v);
}
Var *VarVec::onCopy(Interpreter &vm, const ModuleLoc *loc)
{
	VarVec *tmp = vm.makeVarWithRef<VarVec>(loc, val.size(), asrefs);
	tmp->setVal(vm, val);
	return tmp;
}
void VarVec::onSet(Interpreter &vm, Var *from) { setVal(vm, as<VarVec>(from)->getVal()); }
void VarVec::setVal(Interpreter &vm, Span<Var *> newval)
{
	for(auto &v : val) vm.decVarRef(v);
	val.clear();
	if(asrefs) {
		for(auto &v : newval) {
			vm.incVarRef(v);
		}
		val.assign(newval.begin(), newval.end());
	} else {
		for(auto &v : newval) val.push_back(vm.copyVar(getLoc(), v));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// VarVecIterator ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarVecIterator::VarVecIterator(const ModuleLoc *loc, VarVec *vec)
	: Var(loc, false, false), vec(vec), curr(0)
{}
void VarVecIterator::onCreate(Interpreter &vm) { vm.incVarRef(vec); }
void VarVecIterator::onDestroy(Interpreter &vm) { vm.decVarRef(vec); }
Var *VarVecIterator::onCopy(Interpreter &vm, const ModuleLoc *loc)
{
	return vm.makeVarWithRef<VarVecIterator>(loc, vec);
}
void VarVecIterator::onSet(Interpreter &vm, Var *from)
{
	VarVecIterator *f = as<VarVecIterator>(from);
	vm.decVarRef(vec);
	vm.incVarRef(f->vec);
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

VarMap::VarMap(const ModuleLoc *loc, size_t reservesz, bool asrefs)
	: Var(loc, false, true), asrefs(asrefs)
{
	val.reserve(reservesz);
}
VarMap::VarMap(const ModuleLoc *loc, StringMap<Var *> &&val, bool asrefs)
	: Var(loc, false, true), val(std::move(val)), asrefs(asrefs)
{}
void VarMap::onDestroy(Interpreter &vm) { clear(vm); }
Var *VarMap::onCopy(Interpreter &vm, const ModuleLoc *loc)
{
	VarMap *tmp = vm.makeVarWithRef<VarMap>(loc, val.size(), asrefs);
	tmp->setVal(vm, val);
	return tmp;
}
void VarMap::onSet(Interpreter &vm, Var *from) { setVal(vm, as<VarMap>(from)->getVal()); }
void VarMap::setVal(Interpreter &vm, const StringMap<Var *> &newval)
{
	clear(vm);
	if(asrefs) {
		for(auto &v : newval) {
			vm.incVarRef(v.second);
		}
		val.insert(newval.begin(), newval.end());
	} else {
		for(auto &v : newval) val.insert({v.first, vm.copyVar(getLoc(), v.second)});
	}
}
void VarMap::clear(Interpreter &vm)
{
	for(auto &v : val) vm.decVarRef(v.second);
	val.clear();
}
void VarMap::setAttr(Interpreter &vm, StringRef name, Var *val, bool iref)
{
	auto loc = this->val.find(name);
	if(iref) vm.incVarRef(val);
	if(loc == this->val.end()) {
		this->val.insert({String(name), val});
		return;
	}
	vm.decVarRef(loc->second);
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

VarMapIterator::VarMapIterator(const ModuleLoc *loc, VarMap *map)
	: Var(loc, false, false), map(map), curr(map->getVal().begin())
{}
void VarMapIterator::onCreate(Interpreter &vm) { vm.incVarRef(map); }
void VarMapIterator::onDestroy(Interpreter &vm) { vm.decVarRef(map); }
Var *VarMapIterator::onCopy(Interpreter &vm, const ModuleLoc *loc)
{
	return vm.makeVarWithRef<VarMapIterator>(loc, map);
}
void VarMapIterator::onSet(Interpreter &vm, Var *from)
{
	VarMapIterator *f = as<VarMapIterator>(from);
	vm.decVarRef(map);
	vm.incVarRef(f->map);
	map  = f->map;
	curr = f->curr;
}

bool VarMapIterator::next(Interpreter &vm, const ModuleLoc *loc, Var *&val)
{
	if(curr == map->getVal().end()) return false;
	StringMap<Var *> attrs;
	val = vm.makeVar<VarStruct>(loc, nullptr, 2, typeID<VarMapIterator>());
	as<VarStruct>(val)->setAttr(vm, "0", vm.makeVarWithRef<VarStr>(loc, curr->first), false);
	as<VarStruct>(val)->setAttr(vm, "1", vm.incVarRef(curr->second), false);
	++curr;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarFn /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarFn::VarFn(const ModuleLoc *loc, StringRef modpath, const String &kw_arg, const String &var_arg,
	     size_t paramcount, size_t assn_params_count, FnBody body, bool is_native)
	: Var(loc, true, false), modpath(modpath), kw_arg(kw_arg), var_arg(var_arg), body(body),
	  is_native(is_native)
{
	params.reserve(paramcount);
	assn_params.reserve(assn_params_count);
}
void VarFn::onDestroy(Interpreter &vm)
{
	for(auto &aa : assn_params) vm.decVarRef(aa.second);
}
Var *VarFn::onCopy(Interpreter &vm, const ModuleLoc *loc)
{
	VarFn *tmp = vm.makeVarWithRef<VarFn>(loc, modpath, kw_arg, var_arg, params.size(),
					      assn_params.size(), body, is_native);
	tmp->setParams(params);
	for(auto &aa : assn_params) vm.incVarRef(aa.second);
	tmp->setAssnParams(assn_params);
	return tmp;
}
void VarFn::onSet(Interpreter &vm, Var *from)
{
	VarFn *tmp = as<VarFn>(from);

	modpath = tmp->modpath;
	kw_arg	= tmp->kw_arg;
	var_arg = tmp->var_arg;
	// I assume copy assignment operator of vector & map do not cause unnecessary reallocations
	params = tmp->params;
	for(auto &aa : assn_params) vm.decVarRef(aa.second);
	for(auto &aa : tmp->assn_params) vm.incVarRef(aa.second);
	assn_params = tmp->assn_params;
	body	    = tmp->body;
	is_native   = tmp->is_native;
}
Var *VarFn::call(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		 const StringMap<AssnArgData> &assn_args)
{
	Context &c = vm.getContext();
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
	vm.pushModule(modpath);
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

VarModule::VarModule(const ModuleLoc *loc, Module *mod, Vars *vars, bool is_owner,
		     bool is_thread_copy)
	: Var(loc, false, true), mod(mod), vars(vars), is_owner(is_owner),
	  is_thread_copy(is_thread_copy)
{}
void VarModule::onCreate(Interpreter &vm)
{
	if(vars == nullptr) this->vars = new Vars(vm);
}
void VarModule::onDestroy(Interpreter &vm)
{
	if(is_owner && vars) delete vars;
}
Var *VarModule::onCopy(Interpreter &vm, const ModuleLoc *loc)
{
	return vm.makeVarWithRef<VarModule>(loc, mod, vars, false);
}
void VarModule::onSet(Interpreter &vm, Var *from)
{
	VarModule *m = as<VarModule>(from);
	if(is_owner) delete vars;
	mod	 = m->mod;
	vars	 = m->vars;
	is_owner = false;
}
void VarModule::setAttr(Interpreter &vm, StringRef name, Var *val, bool iref)
{
	vars->add(name, val, iref);
}
bool VarModule::existsAttr(StringRef name) { return vars->exists(name); }
Var *VarModule::getAttr(StringRef name) { return vars->get(name); }

void VarModule::addNativeFn(Interpreter &vm, StringRef name, NativeFn body, size_t args, bool is_va)
{
	VarFn *res = vm.makeVarWithRef<VarFn>(getLoc(), mod->getPath(), "", is_va ? "." : "", args,
					      0, FnBody{.native = body}, true);
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

VarStructDef::VarStructDef(const ModuleLoc *loc, size_t attrscount)
	: Var(loc, true, true), attrorder(attrscount, ""), id(genStructEnumID())
{
	attrs.reserve(attrscount);
}
VarStructDef::VarStructDef(const ModuleLoc *loc, size_t attrscount, size_t id)
	: Var(loc, true, true), attrorder(attrscount, ""), id(id)
{
	attrs.reserve(attrscount);
}
void VarStructDef::onDestroy(Interpreter &vm)
{
	for(auto &attr : attrs) {
		vm.decVarRef(attr.second);
	}
}

Var *VarStructDef::onCopy(Interpreter &vm, const ModuleLoc *loc)
{
	VarStructDef *res = vm.makeVarWithRef<VarStructDef>(loc, attrs.size(), id);
	std::unordered_map<std::string, Var *> attrs;
	for(auto &attr : attrs) {
		res->attrs.insert({attr.first, vm.copyVar(loc, attr.second)});
	}
	res->attrorder = attrorder;
	return res;
}

void VarStructDef::onSet(Interpreter &vm, Var *from)
{
	VarStructDef *st = as<VarStructDef>(from);
	for(auto &attr : attrs) {
		vm.decVarRef(attr.second);
	}
	for(auto &attr : st->attrs) {
		vm.incVarRef(attr.second);
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
		res->setAttr(vm, *it, vm.copyVar(loc, arg), false);
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
		res->setAttr(vm, aa.first, vm.copyVar(loc, aa.second.val), false);
	}

	while(it < attrorder.end()) {
		if(!res->existsAttr(*it)) res->setAttr(vm, *it, vm.copyVar(loc, attrs[*it]), false);
		++it;
	}

	return res;
fail:
	vm.unmakeVar(res);
	return nullptr;
}

void VarStructDef::setAttr(Interpreter &vm, StringRef name, Var *val, bool iref)
{
	auto loc = attrs.find(name);
	if(loc != attrs.end()) {
		vm.decVarRef(loc->second);
	}
	if(iref) vm.incVarRef(val);
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

VarStruct::VarStruct(const ModuleLoc *loc, VarStructDef *base, size_t attrscount)
	: Var(loc, false, true), base(base), id(genStructEnumID())
{
	attrs.reserve(attrscount);
}
VarStruct::VarStruct(const ModuleLoc *loc, VarStructDef *base, size_t attrscount, size_t id)
	: Var(loc, false, true), base(base), id(id)
{
	attrs.reserve(attrscount);
}
void VarStruct::onCreate(Interpreter &vm)
{
	if(base) vm.incVarRef(base);
}
void VarStruct::onDestroy(Interpreter &vm)
{
	for(auto &attr : attrs) {
		vm.decVarRef(attr.second);
	}
	if(base) vm.decVarRef(base);
}
Var *VarStruct::onCopy(Interpreter &vm, const ModuleLoc *loc)
{
	VarStruct *res = vm.makeVarWithRef<VarStruct>(loc, base, attrs.size(), id);
	for(auto &attr : attrs) {
		res->setAttr(vm, attr.first, vm.copyVar(loc, attr.second), false);
	}
	return res;
}

void VarStruct::onSet(Interpreter &vm, Var *from)
{
	VarStruct *st = as<VarStruct>(from);

	for(auto &attr : attrs) {
		vm.decVarRef(attr.second);
	}
	for(auto &attr : st->attrs) {
		vm.incVarRef(attr.second);
		attrs[attr.first] = attr.second;
	}
	if(base) vm.decVarRef(base);
	if(st->base) vm.incVarRef(st->base);
	base = st->base;
	id   = st->id;
}

void VarStruct::setAttr(Interpreter &vm, StringRef name, Var *val, bool iref)
{
	auto loc = attrs.find(name);
	if(loc != attrs.end()) {
		vm.decVarRef(loc->second);
	}
	if(iref) vm.incVarRef(val);
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

VarFile::VarFile(const ModuleLoc *loc, FILE *const file, const String &mode, const bool owner)
	: Var(loc, false, false), file(file), mode(mode), owner(owner)
{}
void VarFile::onDestroy(Interpreter &vm)
{
	if(owner && file) fclose(file);
}

Var *VarFile::onCopy(Interpreter &vm, const ModuleLoc *loc)
{
	return vm.makeVarWithRef<VarFile>(loc, file, mode, false);
}

void VarFile::onSet(Interpreter &vm, Var *from)
{
	if(owner) fclose(file);
	owner = false;
	file  = as<VarFile>(from)->file;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VarFileIterator //////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarFileIterator::VarFileIterator(const ModuleLoc *loc, VarFile *file)
	: Var(loc, false, false), file(file)
{}
void VarFileIterator::onCreate(Interpreter &vm) { vm.incVarRef(file); }
void VarFileIterator::onDestroy(Interpreter &vm) { vm.decVarRef(file); }

Var *VarFileIterator::onCopy(Interpreter &vm, const ModuleLoc *loc)
{
	return vm.makeVarWithRef<VarFileIterator>(loc, file);
}
void VarFileIterator::onSet(Interpreter &vm, Var *from)
{
	vm.decVarRef(file);
	file = as<VarFileIterator>(from)->file;
	vm.incVarRef(file);
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

VarBytebuffer::VarBytebuffer(const ModuleLoc *loc, size_t bufsz, size_t buflen, char *buf)
	: Var(loc, false, false), buffer(nullptr), bufsz(bufsz), buflen(buflen)
{
	if(bufsz > 0) buffer = (char *)malloc(bufsz);
	if(buflen > 0) memcpy(buffer, buf, buflen);
}
VarBytebuffer::~VarBytebuffer()
{
	if(bufsz > 0) free(buffer);
}
Var *VarBytebuffer::onCopy(Interpreter &vm, const ModuleLoc *loc)
{
	return vm.makeVarWithRef<VarBytebuffer>(loc, bufsz, buflen, buffer);
}
void VarBytebuffer::onSet(Interpreter &vm, Var *from)
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