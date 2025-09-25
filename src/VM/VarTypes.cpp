#include "VM/VarTypes.hpp"

#include "VM/Interpreter.hpp"

#if defined(CORE_OS_WINDOWS)
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

Var::Var(ModuleLoc loc, bool callable, bool attr_based) : loc(loc), ref(0), info(0)
{
	if(callable) info |= (size_t)VarInfo::CALLABLE;
	if(attr_based) info |= (size_t)VarInfo::ATTR_BASED;
}
Var::~Var() {}

void Var::create(MemoryManager &mem) { onCreate(mem); }
void Var::destroy(MemoryManager &mem) { onDestroy(mem); }
Var *Var::copy(MemoryManager &mem, ModuleLoc loc) { return onCopy(mem, loc); }
void Var::set(MemoryManager &mem, Var *from) { onSet(mem, from); }

void Var::onCreate(MemoryManager &mem) {}
void Var::onDestroy(MemoryManager &mem) {}
Var *Var::onCopy(MemoryManager &mem, ModuleLoc loc) { return incVarRef(this); }
void Var::onSet(MemoryManager &mem, Var *from) {}

Var *Var::call(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	       const StringMap<AssnArgData> &assn_args)
{
	return nullptr;
}
void Var::setAttr(MemoryManager &mem, StringRef name, Var *val, bool iref) {}
bool Var::existsAttr(StringRef name) { return false; }
Var *Var::getAttr(StringRef name) { return nullptr; }
size_t Var::getTypeFnID() { return getType(); }

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
Var *VarTypeID::onCopy(MemoryManager &mem, ModuleLoc loc)
{
	return makeVarWithRef<VarTypeID>(mem, loc, val);
}
void VarTypeID::onSet(MemoryManager &mem, Var *from) { val = as<VarTypeID>(from)->getVal(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarBool ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarBool::VarBool(ModuleLoc loc, bool val) : Var(loc, false, false), val(val) {}
Var *VarBool::onCopy(MemoryManager &mem, ModuleLoc loc)
{
	return makeVarWithRef<VarBool>(mem, loc, val);
}
void VarBool::onSet(MemoryManager &mem, Var *from) { val = as<VarBool>(from)->getVal(); }

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarInt ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarInt::VarInt(ModuleLoc loc, int64_t _val) : Var(loc, false, false), val(_val) {}
VarInt::VarInt(ModuleLoc loc, const char *_val) : Var(loc, false, false), val(std::stoll(_val)) {}
Var *VarInt::onCopy(MemoryManager &mem, ModuleLoc loc)
{
	return makeVarWithRef<VarInt>(mem, loc, val);
}
void VarInt::onSet(MemoryManager &mem, Var *from) { val = as<VarInt>(from)->getVal(); }

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

Var *VarIntIterator::onCopy(MemoryManager &mem, ModuleLoc loc)
{
	return makeVarWithRef<VarIntIterator>(mem, loc, begin, end, step);
}
void VarIntIterator::onSet(MemoryManager &mem, Var *from)
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
Var *VarFlt::onCopy(MemoryManager &mem, ModuleLoc loc)
{
	return makeVarWithRef<VarFlt>(mem, loc, val);
}
void VarFlt::onSet(MemoryManager &mem, Var *from) { val = as<VarFlt>(from)->getVal(); }

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
Var *VarStr::onCopy(MemoryManager &mem, ModuleLoc loc)
{
	return makeVarWithRef<VarStr>(mem, loc, val);
}
void VarStr::onSet(MemoryManager &mem, Var *from) { val = as<VarStr>(from)->getVal(); }

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
void VarVec::onDestroy(MemoryManager &mem)
{
	for(auto &v : val) decVarRef(mem, v);
}
Var *VarVec::onCopy(MemoryManager &mem, ModuleLoc loc)
{
	VarVec *tmp = makeVarWithRef<VarVec>(mem, loc, val.size(), asrefs);
	tmp->setVal(mem, val);
	return tmp;
}
void VarVec::onSet(MemoryManager &mem, Var *from) { setVal(mem, as<VarVec>(from)->getVal()); }
void VarVec::setVal(MemoryManager &mem, Span<Var *> newval)
{
	for(auto &v : val) decVarRef(mem, v);
	val.clear();
	if(asrefs) {
		for(auto &v : newval) {
			incVarRef(v);
		}
		val.assign(newval.begin(), newval.end());
	} else {
		for(auto &v : newval) val.push_back(copyVar(mem, getLoc(), v));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// VarVecIterator ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarVecIterator::VarVecIterator(ModuleLoc loc, VarVec *vec)
	: Var(loc, false, false), vec(vec), curr(0)
{}
void VarVecIterator::onCreate(MemoryManager &mem) { incVarRef(vec); }
void VarVecIterator::onDestroy(MemoryManager &mem) { decVarRef(mem, vec); }
Var *VarVecIterator::onCopy(MemoryManager &mem, ModuleLoc loc)
{
	return makeVarWithRef<VarVecIterator>(mem, loc, vec);
}
void VarVecIterator::onSet(MemoryManager &mem, Var *from)
{
	VarVecIterator *f = as<VarVecIterator>(from);
	decVarRef(mem, vec);
	incVarRef(f->vec);
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
void VarMap::onDestroy(MemoryManager &mem) { clear(mem); }
Var *VarMap::onCopy(MemoryManager &mem, ModuleLoc loc)
{
	VarMap *tmp = makeVarWithRef<VarMap>(mem, loc, val.size(), asrefs);
	tmp->setVal(mem, val);
	return tmp;
}
void VarMap::onSet(MemoryManager &mem, Var *from) { setVal(mem, as<VarMap>(from)->getVal()); }
void VarMap::setVal(MemoryManager &mem, const StringMap<Var *> &newval)
{
	clear(mem);
	if(asrefs) {
		for(auto &v : newval) {
			incVarRef(v.second);
		}
		val.insert(newval.begin(), newval.end());
	} else {
		for(auto &v : newval) val.insert({v.first, copyVar(mem, getLoc(), v.second)});
	}
}
void VarMap::clear(MemoryManager &mem)
{
	for(auto &v : val) decVarRef(mem, v.second);
	val.clear();
}
void VarMap::setAttr(MemoryManager &mem, StringRef name, Var *val, bool iref)
{
	auto loc = this->val.find(name);
	if(iref) incVarRef(val);
	if(loc == this->val.end()) {
		this->val.insert({String(name), val});
		return;
	}
	decVarRef(mem, loc->second);
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
void VarMapIterator::onCreate(MemoryManager &mem) { incVarRef(map); }
void VarMapIterator::onDestroy(MemoryManager &mem) { decVarRef(mem, map); }
Var *VarMapIterator::onCopy(MemoryManager &mem, ModuleLoc loc)
{
	return makeVarWithRef<VarMapIterator>(mem, loc, map);
}
void VarMapIterator::onSet(MemoryManager &mem, Var *from)
{
	VarMapIterator *f = as<VarMapIterator>(from);
	decVarRef(mem, map);
	incVarRef(f->map);
	map  = f->map;
	curr = f->curr;
}

bool VarMapIterator::next(MemoryManager &mem, ModuleLoc loc, Var *&val)
{
	if(curr == map->getVal().end()) return false;
	StringMap<Var *> attrs;
	val = makeVar<VarStruct>(mem, loc, nullptr, 2, typeID<VarMapIterator>());
	as<VarStruct>(val)->setAttr(mem, "0", makeVarWithRef<VarStr>(mem, loc, curr->first), false);
	as<VarStruct>(val)->setAttr(mem, "1", incVarRef(curr->second), false);
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
void VarFn::onDestroy(MemoryManager &mem)
{
	for(auto &aa : assn_params) decVarRef(mem, aa.second);
}
Var *VarFn::call(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
		 const StringMap<AssnArgData> &assn_args)
{
	MemoryManager &mem = vm.getMemoryManager();
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
	Vars &vars = vm.getVars();
	// take care of 'self' (always present - either data or nullptr)
	if(args[0] != nullptr) vars.stash("self", args[0]);
	// default arguments
	Set<StringRef> found_args;
	size_t i = 1;
	for(auto &a : params) {
		if(i == args.size()) break;
		vars.stash(a, args[i++]);
		found_args.insert(a);
	}
	// add all default args which have not been overwritten by args
	for(auto &aa : assn_params) {
		if(found_args.find(aa.first) != found_args.end()) continue;
		Var *cpy = copyVar(mem, loc, aa.second);
		vars.stash(aa.first, cpy, false); // copy will make sure there is ref = 1 already
	}
	if(!var_arg.empty()) {
		VarVec *v = makeVarWithRef<VarVec>(mem, loc, args.size(), false);
		while(i < args.size()) {
			incVarRef(args[i]);
			v->push(args[i]);
			++i;
		}
		vars.stash(var_arg, v, false);
	}
	if(!kw_arg.empty()) {
		VarMap *m = makeVarWithRef<VarMap>(mem, loc, assn_args.size(), false);
		m->initializePos(assn_args.size());
		for(auto &a : assn_args) {
			incVarRef(a.second.val);
			m->insert(a.first, a.second.val);
			m->setPos(a.second.pos, a.first);
		}
		vars.stash(kw_arg, m, false);
	}
	if(vm.execute(true, false, body.feral.begin, body.feral.end) != 0 && !vm.isExitCalled()) {
		vars.unstash();
		vm.popModule();
		return nullptr;
	}
	vm.popModule();
	return vm.getNil();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// VarModule ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarModule::VarModule(ModuleLoc loc, StringRef path, Bytecode &&bc, ModuleId moduleId,
		     VarStack *varStack)
	: Var(loc, false, true), path(path), bc(std::move(bc)), moduleId(moduleId),
	  varStack(varStack), ownsVars(varStack == nullptr)
{}
void VarModule::onCreate(MemoryManager &mem)
{
	if(varStack == nullptr) this->varStack = new VarStack(mem);
}
void VarModule::onDestroy(MemoryManager &mem)
{
	if(varStack && ownsVars) delete varStack;
}
void VarModule::setAttr(MemoryManager &mem, StringRef name, Var *val, bool iref)
{
	varStack->add(name, val, iref);
}
bool VarModule::existsAttr(StringRef name) { return varStack->exists(name); }
Var *VarModule::getAttr(StringRef name) { return varStack->get(name); }

void VarModule::addNativeFn(VirtualMachine &vm, StringRef name, NativeFn body, size_t args,
			    bool is_va)
{
	return addNativeFn(vm.getMemoryManager(), name, body, args, is_va);
}
void VarModule::addNativeFn(MemoryManager &mem, StringRef name, NativeFn body, size_t args,
			    bool is_va)
{
	VarFn *res = makeVarWithRef<VarFn>(mem, getLoc(), moduleId, "", is_va ? "." : "", args, 0,
					   FnBody{.native = body}, true);
	for(size_t i = 0; i < args; ++i) res->pushParam("");
	varStack->add(name, res, false);
}
void VarModule::addNativeVar(StringRef name, Var *val, bool iref)
{
	varStack->add(name, val, iref);
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
void VarStructDef::onDestroy(MemoryManager &mem)
{
	for(auto &attr : attrs) {
		decVarRef(mem, attr.second);
	}
}

Var *VarStructDef::call(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
			const StringMap<AssnArgData> &assn_args)
{
	MemoryManager &mem = vm.getMemoryManager();
	for(auto &aa : assn_args) {
		if(std::find(attrorder.begin(), attrorder.end(), aa.first) == attrorder.end()) {
			vm.fail(aa.second.val->getLoc(), "no attribute named '", aa.first,
				"' in the structure definition");
			return nullptr;
		}
	}

	VarStruct *res = makeVar<VarStruct>(mem, loc, this, attrs.size(), id);

	auto it = attrorder.begin();
	for(auto argit = args.begin() + 1; argit != args.end(); ++argit) {
		auto &arg = *argit;
		if(it == attrorder.end()) {
			vm.fail(arg->getLoc(),
				"provided more arguments than existing in structure definition");
			goto fail;
		}
		res->setAttr(mem, *it, copyVar(mem, loc, arg), false);
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
		res->setAttr(mem, aa.first, copyVar(mem, loc, aa.second.val), false);
	}

	while(it < attrorder.end()) {
		if(!res->existsAttr(*it))
			res->setAttr(mem, *it, copyVar(mem, loc, attrs[*it]), false);
		++it;
	}

	return res;
fail:
	vm.unmakeVar(res);
	return nullptr;
}

void VarStructDef::setAttr(MemoryManager &mem, StringRef name, Var *val, bool iref)
{
	auto loc = attrs.find(name);
	if(loc != attrs.end()) {
		decVarRef(mem, loc->second);
	}
	if(iref) incVarRef(val);
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
void VarStruct::onCreate(MemoryManager &mem)
{
	if(base) incVarRef(base);
}
void VarStruct::onDestroy(MemoryManager &mem)
{
	for(auto &attr : attrs) {
		decVarRef(mem, attr.second);
	}
	if(base) decVarRef(mem, base);
}
Var *VarStruct::onCopy(MemoryManager &mem, ModuleLoc loc)
{
	VarStruct *res = makeVarWithRef<VarStruct>(mem, loc, base, attrs.size(), id);
	for(auto &attr : attrs) {
		res->setAttr(mem, attr.first, copyVar(mem, loc, attr.second), false);
	}
	return res;
}

void VarStruct::onSet(MemoryManager &mem, Var *from)
{
	VarStruct *st = as<VarStruct>(from);

	for(auto &attr : attrs) {
		decVarRef(mem, attr.second);
	}
	for(auto &attr : st->attrs) {
		incVarRef(attr.second);
		attrs[attr.first] = attr.second;
	}
	if(base) decVarRef(mem, base);
	if(st->base) incVarRef(st->base);
	base = st->base;
	id   = st->id;
}

void VarStruct::setAttr(MemoryManager &mem, StringRef name, Var *val, bool iref)
{
	auto loc = attrs.find(name);
	if(loc != attrs.end()) {
		decVarRef(mem, loc->second);
	}
	if(iref) incVarRef(val);
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
void VarFile::onDestroy(MemoryManager &mem)
{
	if(owner && file) fclose(file);
}

Var *VarFile::onCopy(MemoryManager &mem, ModuleLoc loc)
{
	return makeVarWithRef<VarFile>(mem, loc, file, mode, false);
}

void VarFile::onSet(MemoryManager &mem, Var *from)
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
void VarFileIterator::onCreate(MemoryManager &mem) { incVarRef(file); }
void VarFileIterator::onDestroy(MemoryManager &mem) { decVarRef(mem, file); }

Var *VarFileIterator::onCopy(MemoryManager &mem, ModuleLoc loc)
{
	return makeVarWithRef<VarFileIterator>(mem, loc, file);
}
void VarFileIterator::onSet(MemoryManager &mem, Var *from)
{
	decVarRef(mem, file);
	file = as<VarFileIterator>(from)->file;
	incVarRef(file);
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
Var *VarBytebuffer::onCopy(MemoryManager &mem, ModuleLoc loc)
{
	return makeVarWithRef<VarBytebuffer>(mem, loc, bufsz, buflen, buffer);
}
void VarBytebuffer::onSet(MemoryManager &mem, Var *from)
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