#include "VM/VarTypes.hpp"

#include "VM/Interpreter.hpp"
#include "VM/VarMemory.hpp"

namespace fer
{

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// Var ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var::Var(const ModuleLoc *loc, uiptr _typeid, bool callable, bool attr_based)
	: loc(loc), _typeid(_typeid), ref(1), info(0)
{
	if(callable) info |= (size_t)VarInfo::CALLABLE;
	if(attr_based) info |= (size_t)VarInfo::ATTR_BASED;
}
Var::~Var() {}

uiptr Var::getTypeFnID() { return _typeid; }

Var *Var::call(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<String, AssnArgData> &assn_args)
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

VarAll::VarAll(const ModuleLoc *loc) : Var(loc, typeID<VarAll>(), false, false) {}
Var *VarAll::copy(const ModuleLoc *loc) { return new VarAll(loc); }
void VarAll::set(Var *from) {}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VarNil ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarNil::VarNil(const ModuleLoc *loc) : Var(loc, typeID<VarNil>(), false, false) {}
Var *VarNil::copy(const ModuleLoc *loc)
{
	this->iref();
	return this;
}
void VarNil::set(Var *from) {}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// VarTypeID //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarTypeID::VarTypeID(const ModuleLoc *loc, uiptr val)
	: Var(loc, typeID<VarTypeID>(), false, false), val(val)
{}
Var *VarTypeID::copy(const ModuleLoc *loc) { return new VarTypeID(loc, val); }
void VarTypeID::set(Var *from) { val = as<VarTypeID>(from)->get(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarBool ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarBool::VarBool(const ModuleLoc *loc, bool val)
	: Var(loc, typeID<VarBool>(), false, false), val(val)
{}
Var *VarBool::copy(const ModuleLoc *loc) { return new VarBool(loc, val); }
void VarBool::set(Var *from) { val = as<VarBool>(from)->get(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarInt ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarInt::VarInt(const ModuleLoc *loc, int64_t _val)
	: Var(loc, typeID<VarInt>(), false, false), val(_val)
{}
VarInt::VarInt(const ModuleLoc *loc, const char *_val)
	: Var(loc, typeID<VarInt>(), false, false), val(std::stoll(_val))
{}
VarInt::~VarInt() {}
Var *VarInt::copy(const ModuleLoc *loc) { return new VarInt(loc, val); }
void VarInt::set(Var *from) { val = as<VarInt>(from)->get(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarFlt ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarFlt::VarFlt(const ModuleLoc *loc, long double _val)
	: Var(loc, typeID<VarFlt>(), false, false), val(_val)
{}
VarFlt::VarFlt(const ModuleLoc *loc, const char *_val)
	: Var(loc, typeID<VarFlt>(), false, false), val(std::stod(_val))
{}
VarFlt::~VarFlt() {}
Var *VarFlt::copy(const ModuleLoc *loc) { return new VarFlt(loc, val); }
void VarFlt::set(Var *from) { val = as<VarFlt>(from)->get(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarStr ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarStr::VarStr(const ModuleLoc *loc, char val)
	: Var(loc, typeID<VarStr>(), false, false), val(1, val)
{}
VarStr::VarStr(const ModuleLoc *loc, StringRef val)
	: Var(loc, typeID<VarStr>(), false, false), val(val)
{}
VarStr::VarStr(const ModuleLoc *loc, InitList<StringRef> _val)
	: Var(loc, typeID<VarStr>(), false, false)
{
	for(auto &e : _val) val += e;
}
VarStr::VarStr(const ModuleLoc *loc, const char *val, size_t count)
	: Var(loc, typeID<VarStr>(), false, false), val(val, count)
{}
Var *VarStr::copy(const ModuleLoc *loc) { return new VarStr(loc, val); }
void VarStr::set(Var *from) { val = as<VarStr>(from)->get(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarVec ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarVec::VarVec(const ModuleLoc *loc, size_t reservesz, bool asrefs)
	: Var(loc, typeID<VarVec>(), false, false), asrefs(asrefs)
{
	val.reserve(reservesz);
}
VarVec::VarVec(const ModuleLoc *loc, Vector<Var *> &&val, bool asrefs)
	: Var(loc, typeID<VarVec>(), false, false), val(std::move(val)), asrefs(asrefs)
{}
VarVec::~VarVec()
{
	for(auto &v : val) decref(v);
}
Var *VarVec::copy(const ModuleLoc *loc)
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
////////////////////////////////////////// VarMap ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarMap::VarMap(const ModuleLoc *loc, size_t reservesz, bool asrefs)
	: Var(loc, typeID<VarMap>(), false, false), asrefs(asrefs)
{
	val.reserve(reservesz);
}
VarMap::VarMap(const ModuleLoc *loc, StringMap<Var *> &&val, bool asrefs)
	: Var(loc, typeID<VarMap>(), false, false), val(std::move(val)), asrefs(asrefs)
{}
VarMap::~VarMap()
{
	for(auto &v : val) decref(v.second);
}
Var *VarMap::copy(const ModuleLoc *loc)
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

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarFn /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarFn::VarFn(const ModuleLoc *loc, StringRef modpath, const String &kw_arg, const String &var_arg,
	     size_t paramcount, size_t assn_params_count, FnBody body, bool is_native)
	: Var(loc, typeID<VarFn>(), true, false), modpath(modpath), kw_arg(kw_arg),
	  var_arg(var_arg), body(body), is_native(is_native)
{
	params.reserve(paramcount);
	assn_params.reserve(assn_params_count);
}
VarFn::~VarFn()
{
	for(auto &aa : assn_params) decref(aa.second);
}
Var *VarFn::copy(const ModuleLoc *loc)
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
		 const Map<String, AssnArgData> &assn_args)
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
		for(auto &a : assn_args) {
			incref(a.second.val);
			m->insert(a.first, a.second.val);
		}
		vars->stash(kw_arg, m, false);
	}
	if(vm.execute(nullptr, body.feral.begin, body.feral.end) != 0 && !vm.isExitCalled()) {
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
	: Var(loc, typeID<VarModule>(), false, true), mod(mod), vars(vars), is_owner(is_owner),
	  is_thread_copy(is_thread_copy)
{
	if(vars == nullptr) this->vars = new Vars;
}

VarModule::~VarModule()
{
	if(is_owner && vars) delete vars;
}

Var *VarModule::copy(const ModuleLoc *loc) { return new VarModule(loc, mod, vars, false); }
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

} // namespace fer