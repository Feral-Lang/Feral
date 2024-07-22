#include "VM/VarTypes.hpp"

#include "VM/Interpreter.hpp"
#include "VM/VarMemory.hpp"

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

size_t Var::getTypeFnID() { return getType(); }

Var *Var::copy(const ModuleLoc *loc)
{
	if(isLoadAsRef()) {
		unsetLoadAsRef();
		incref(this);
		return this;
	}
	return this->copyImpl(loc);
}

Var *Var::call(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const StringMap<AssnArgData> &assn_args)
{
	return nullptr;
}
void Var::setAttr(StringRef name, Var *val, bool iref) {}
bool Var::existsAttr(StringRef name) { return false; }
Var *Var::getAttr(StringRef name) { return nullptr; }

void *Var::operator new(size_t sz) { return VarMemory::getInstance().alloc(sz); }
void Var::operator delete(void *ptr, size_t sz) { VarMemory::getInstance().free(ptr, sz); }

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VarAll ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarAll::VarAll(const ModuleLoc *loc) : Var(loc, false, false) {}
Var *VarAll::copyImpl(const ModuleLoc *loc) { return new VarAll(loc); }
void VarAll::set(Var *from) {}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VarNil ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarNil::VarNil(const ModuleLoc *loc) : Var(loc, false, false) {}
Var *VarNil::copyImpl(const ModuleLoc *loc)
{
	this->iref();
	return this;
}
void VarNil::set(Var *from) {}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// VarTypeID //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarTypeID::VarTypeID(const ModuleLoc *loc, size_t val) : Var(loc, false, false), val(val) {}
Var *VarTypeID::copyImpl(const ModuleLoc *loc) { return new VarTypeID(loc, val); }
void VarTypeID::set(Var *from) { val = as<VarTypeID>(from)->get(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarBool ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarBool::VarBool(const ModuleLoc *loc, bool val) : Var(loc, false, false), val(val) {}
Var *VarBool::copyImpl(const ModuleLoc *loc) { return new VarBool(loc, val); }
void VarBool::set(Var *from) { val = as<VarBool>(from)->get(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarInt ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarInt::VarInt(const ModuleLoc *loc, int64_t _val) : Var(loc, false, false), val(_val) {}
VarInt::VarInt(const ModuleLoc *loc, const char *_val)
	: Var(loc, false, false), val(std::stoll(_val))
{}
VarInt::~VarInt() {}
Var *VarInt::copyImpl(const ModuleLoc *loc) { return new VarInt(loc, val); }
void VarInt::set(Var *from) { val = as<VarInt>(from)->get(); }

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
VarIntIterator::~VarIntIterator() {}

Var *VarIntIterator::copyImpl(const ModuleLoc *loc)
{
	return new VarIntIterator(loc, begin, end, step);
}
void VarIntIterator::set(Var *from)
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
	: Var(loc, false, false), val(std::stod(_val))
{}
VarFlt::~VarFlt() {}
Var *VarFlt::copyImpl(const ModuleLoc *loc) { return new VarFlt(loc, val); }
void VarFlt::set(Var *from) { val = as<VarFlt>(from)->get(); }

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
Var *VarStr::copyImpl(const ModuleLoc *loc) { return new VarStr(loc, val); }
void VarStr::set(Var *from) { val = as<VarStr>(from)->get(); }

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
VarVec::~VarVec()
{
	for(auto &v : val) decref(v);
}
Var *VarVec::copyImpl(const ModuleLoc *loc)
{
	VarVec *tmp = new VarVec(loc, val.size(), asrefs);
	if(asrefs) {
		for(auto &v : val) incref(v);
		tmp->set(val);
	} else {
		for(auto &v : val) tmp->push(v->copy(loc));
	}
	return tmp;
}
void VarVec::set(Span<Var *> newval)
{
	for(auto &v : val) decref(v);
	val.clear();
	if(asrefs) {
		for(auto &v : newval) {
			incref(v);
		}
		val.assign(newval.begin(), newval.end());
	} else {
		for(auto &v : newval) val.push_back(v->copy(getLoc()));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// VarVecIterator ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarVecIterator::VarVecIterator(const ModuleLoc *loc, VarVec *vec)
	: Var(loc, false, false), vec(vec), curr(0)
{
	incref(vec);
}
VarVecIterator::~VarVecIterator() { decref(vec); }

Var *VarVecIterator::copyImpl(const ModuleLoc *loc) { return new VarVecIterator(loc, vec); }
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
VarMap::~VarMap()
{
	for(auto &v : val) decref(v.second);
}
Var *VarMap::copyImpl(const ModuleLoc *loc)
{
	VarMap *tmp = new VarMap(loc, val.size(), asrefs);
	if(asrefs) {
		for(auto &v : val) incref(v.second);
		tmp->set(val);
	} else {
		for(auto &v : val) tmp->insert(v.first, v.second->copy(loc));
	}
	return tmp;
}
void VarMap::set(const StringMap<Var *> &newval)
{
	for(auto &v : val) decref(v.second);
	val.clear();
	if(asrefs) {
		for(auto &v : newval) {
			incref(v.second);
		}
		val.insert(newval.begin(), newval.end());
	} else {
		for(auto &v : newval) val.insert({v.first, v.second->copy(getLoc())});
	}
}
void VarMap::setAttr(StringRef name, Var *val, bool iref)
{
	auto loc = this->val.find(name);
	if(iref) incref(val);
	if(loc == this->val.end()) {
		this->val.insert({String(name), val});
		return;
	}
	decref(loc->second);
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
	: Var(loc, false, false), map(map), curr(map->get().begin())
{
	incref(map);
}
VarMapIterator::~VarMapIterator() { decref(map); }

Var *VarMapIterator::copyImpl(const ModuleLoc *loc) { return new VarMapIterator(loc, map); }
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
VarFn::~VarFn()
{
	for(auto &aa : assn_params) decref(aa.second);
}
Var *VarFn::copyImpl(const ModuleLoc *loc)
{
	VarFn *tmp = new VarFn(loc, modpath, kw_arg, var_arg, params.size(), assn_params.size(),
			       body, is_native);
	tmp->setParams(params);
	for(auto &aa : assn_params) incref(aa.second);
	tmp->setAssnParams(assn_params);
	return tmp;
}
void VarFn::set(Var *from)
{
	VarFn *tmp = as<VarFn>(from);

	modpath = tmp->modpath;
	kw_arg	= tmp->kw_arg;
	var_arg = tmp->var_arg;
	// I assume copy assignment operator of vector & map do not cause unnecessary reallocations
	params = tmp->params;
	for(auto &aa : assn_params) decref(aa.second);
	for(auto &aa : tmp->assn_params) incref(aa.second);
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
		Var *cpy = aa.second->copy(loc);
		vars->stash(aa.first, cpy, false); // copy will make sure there is ref = 1 already
	}
	if(!var_arg.empty()) {
		VarVec *v = vm.makeVarWithRef<VarVec>(loc, args.size(), false);
		while(i < args.size()) {
			incref(args[i]);
			v->push(args[i]);
			++i;
		}
		vars->stash(var_arg, v, false);
	}
	if(!kw_arg.empty()) {
		VarMap *m = vm.makeVarWithRef<VarMap>(loc, assn_args.size(), false);
		m->initializePos(assn_args.size());
		for(auto &a : assn_args) {
			incref(a.second.val);
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
{
	if(vars == nullptr) this->vars = new Vars;
}

VarModule::~VarModule()
{
	if(is_owner && vars) delete vars;
}

Var *VarModule::copyImpl(const ModuleLoc *loc) { return new VarModule(loc, mod, vars, false); }
Var *VarModule::threadCopy(const ModuleLoc *loc)
{
	return new VarModule(loc, mod, vars->threadCopy(loc), true, true);
}
void VarModule::set(Var *from)
{
	VarModule *m = as<VarModule>(from);
	if(is_owner) delete vars;
	mod	 = m->mod;
	vars	 = m->vars;
	is_owner = false;
}

void VarModule::setAttr(StringRef name, Var *val, bool iref) { vars->add(name, val, iref); }
bool VarModule::existsAttr(StringRef name) { return vars->exists(name); }
Var *VarModule::getAttr(StringRef name) { return vars->get(name); }

void VarModule::addNativeFn(StringRef name, NativeFn body, size_t args, bool is_va)
{
	VarFn *res = new VarFn(getLoc(), mod->getPath(), "", is_va ? "." : "", args, 0,
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

VarStructDef::~VarStructDef()
{
	for(auto &attr : attrs) {
		decref(attr.second);
	}
}

Var *VarStructDef::copyImpl(const ModuleLoc *loc)
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

VarStruct::VarStruct(const ModuleLoc *loc, VarStructDef *base, size_t attrscount)
	: Var(loc, false, true), base(base), id(genStructEnumID())
{
	if(base) incref(base);
	attrs.reserve(attrscount);
}

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

Var *VarStruct::copyImpl(const ModuleLoc *loc)
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

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// VarFile ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarFile::VarFile(const ModuleLoc *loc, FILE *const file, const String &mode, const bool owner)
	: Var(loc, false, false), file(file), mode(mode), owner(owner)
{}
VarFile::~VarFile()
{
	if(owner && file) fclose(file);
}

Var *VarFile::copyImpl(const ModuleLoc *loc) { return new VarFile(loc, file, mode, false); }

void VarFile::set(Var *from)
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
{
	incref(file);
}
VarFileIterator::~VarFileIterator() { decref(file); }

Var *VarFileIterator::copyImpl(const ModuleLoc *loc) { return new VarFileIterator(loc, file); }
void VarFileIterator::set(Var *from)
{
	decref(file);
	file = as<VarFileIterator>(from)->file;
	incref(file);
}

bool VarFileIterator::next(VarStr *&val)
{
	if(!val) return false;
	char *lineptr	= NULL;
	size_t len	= 0;
	String &valdata = val->get();
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

VarBytebuffer::VarBytebuffer(const ModuleLoc *loc, size_t bufsz)
	: Var(loc, false, false), buffer(nullptr), bufsz(bufsz), buflen(0)
{
	if(bufsz > 0) buffer = (char *)malloc(bufsz);
}
VarBytebuffer::~VarBytebuffer()
{
	if(bufsz > 0) free(buffer);
}

Var *VarBytebuffer::copyImpl(const ModuleLoc *loc)
{
	VarBytebuffer *newbuf = new VarBytebuffer(loc, bufsz);
	newbuf->set(this);
	return newbuf;
}

void VarBytebuffer::set(Var *from)
{
	VarBytebuffer *tmp = as<VarBytebuffer>(from);
	if(tmp->bufsz == 0) {
		if(bufsz > 0) free(buffer);
		bufsz = 0;
		return;
	}
	if(bufsz != tmp->bufsz) {
		if(bufsz == 0) buffer = (char *)malloc(tmp->bufsz);
		else buffer = (char *)realloc(buffer, tmp->bufsz);
	}
	memcpy(buffer, tmp->buffer, tmp->bufsz);
	bufsz  = tmp->bufsz;
	buflen = tmp->buflen;
}

void VarBytebuffer::resize(size_t newsz)
{
	if(newsz == 0) {
		if(bufsz > 0) free(buffer);
		bufsz = 0;
		return;
	}
	if(bufsz == 0) buffer = (char *)malloc(newsz);
	else buffer = (char *)realloc(buffer, newsz);
	bufsz = newsz;
}

} // namespace fer