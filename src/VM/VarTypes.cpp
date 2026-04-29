#include "VM/VarTypes.hpp"

#include "VM/VM.hpp"

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

Var::Var(ModuleLoc loc, size_t infoFlags) : loc(loc), ref(0), info(infoFlags), doc(nullptr) {}
Var::~Var() {}

void Var::create(VirtualMachine &vm)
{
    if(isCreated()) {
        vm.fail(loc, "attempted to recreate: ", vm.getTypeName(this));
        return;
    }
    onCreate(vm);
    setCreated();
}
void Var::destroy(VirtualMachine &vm)
{
    if(!isCreated()) return;
    unsetCreated();
    onDestroy(vm);
    if(doc) vm.decVarRef(doc);
}
void Var::init(VirtualMachine &vm)
{
    if(!vm.isReady() || isBasic()) return;
    if(isInitialized()) {
        vm.fail(loc, "attempted to reinitialize: ", vm.getTypeName(this));
        return;
    }
    Var *fn = vm.getTypeFn(this, "_init_");
    if(!fn) return;
    if(!fn->isCallable()) {
        vm.fail(loc, "`_init_` is not a callable for: ", vm.getTypeName(this),
                "; it is: ", vm.getTypeName(fn));
        return;
    }
    Array<Var *, 1> args = {this};
    Var *ret             = nullptr;
    iref();
    bool res = vm.callVarAndExpect<VarNil>(loc, "_init_", fn, ret, args, {});
    dref();
    if(!res) {
        vm.fail(loc, "failed to init var: ", vm.getTypeName(this));
        return;
    }
    vm.decVarRef(ret);
    setInitialized();
}
void Var::deinit(VirtualMachine &vm)
{
    if(!vm.isReady() || isBasic() || !isInitialized()) return;
    unsetInitialized();
    Var *fn = vm.getTypeFn(this, "_deinit_");
    if(!fn) return;
    if(!fn->isCallable()) {
        vm.fail(loc, "`_deinit_` is not a callable for: ", vm.getTypeName(this),
                "; it is: ", vm.getTypeName(fn));
        return;
    }
    Array<Var *, 1> args = {this};
    Var *ret             = nullptr;
    iref();
    bool res = vm.callVarAndExpect<VarNil>(loc, "_deinit_", fn, ret, args, {});
    dref();
    if(res) vm.decVarRef(ret);
    else vm.fail(loc, "failed to deinit var: ", vm.getTypeName(this));
}
Var *Var::copy(VirtualMachine &vm, ModuleLoc loc)
{
    if(isLoadAsRef()) {
        unsetLoadAsRef();
        return vm.incVarRef(this);
    }
    Var *fn = vm.getTypeFn(this, "_copy_");
    if(!fn) return vm.incVarRef(this);
    if(!fn->isCallable()) return nullptr;
    Array<Var *, 1> args = {this};
    Var *res             = vm.callVar(loc, "_copy_", fn, args, nullptr);
    if(doc && res) res->setDoc(vm, doc);
    return res;
}
bool Var::set(VirtualMachine &vm, Var *from)
{
    if(from->doc) setDoc(vm, from->doc);
    return onSet(vm, from);
}
Var *Var::call(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args, VarMap *assnArgs, VarVec *stack,
               size_t *currentlyAt)
{
    return onCall(vm, loc, args, assnArgs, stack, currentlyAt);
}

void Var::onCreate(VirtualMachine &vm) {}
void Var::onDestroy(VirtualMachine &vm) {}
bool Var::onSet(VirtualMachine &vm, Var *from) { return true; }
Var *Var::onCall(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args, VarMap *assnArgs,
                 VarVec *stack, size_t *currentlyAt)
{
    return nullptr;
}

void Var::setAttr(VirtualMachine &vm, StringRef name, Var *val, bool iref) {}
void Var::remAttr(VirtualMachine &vm, StringRef name, bool &found, bool dref) {}
bool Var::existsAttr(StringRef name) { return false; }
Var *Var::getAttr(StringRef name) { return nullptr; }
void Var::getAttrList(VirtualMachine &vm, VarVec *dest) {}
size_t Var::getAttrCount() { return 0; }
size_t Var::getSubType() { return getType(); }

void Var::setDoc(VirtualMachine &vm, VarStr *newDoc)
{
    if(newDoc) vm.incVarRef(newDoc);
    if(doc) vm.decVarRef(doc);
    doc = newDoc;
}

void Var::setDoc(VirtualMachine &vm, ModuleLoc loc, StringRef newDoc)
{
    if(doc) vm.decVarRef(doc);
    if(newDoc.empty()) return;
    doc = vm.incVarRef(vm.makeVar<VarStr>(loc, newDoc));
}

void Var::dump(String &outStr, VirtualMachine *vm)
{
    outStr += "<";
    outStr += std::to_string(ref);
    outStr += ">";
    if(is<VarNil>()) {
        outStr += "Nil";
    } else if(is<VarBool>()) {
        outStr += "Bool:";
        outStr += (as<VarBool>(this)->getVal() ? "true" : "false");
    } else if(is<VarInt>()) {
        outStr += "Int:";
        outStr += std::to_string(as<VarInt>(this)->getVal());
    } else if(is<VarFlt>()) {
        outStr += "Flt:";
        outStr += std::to_string(as<VarFlt>(this)->getVal());
    } else if(is<VarStr>()) {
        outStr += "Str:";
        outStr += as<VarStr>(this)->getVal();
    } else if(is<VarPath>()) {
        outStr += "Path:";
        outStr += as<VarPath>(this)->toStr();
    } else if(is<VarVec>()) {
        outStr += "Vec:";
        outStr += std::to_string(as<VarVec>(this)->size());
    } else if(is<VarMap>()) {
        outStr += "Map:";
        outStr += std::to_string(as<VarMap>(this)->size());
    } else if(vm) {
        outStr += vm->getTypeName(this);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VarAll ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarAll::VarAll(ModuleLoc loc) : Var(loc, VarInfo::BASIC) {}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VarNil ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarNil::VarNil(ModuleLoc loc) : Var(loc, VarInfo::BASIC) {}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// VarTypeID //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarTypeID::VarTypeID(ModuleLoc loc, size_t val) : Var(loc, VarInfo::BASIC), val(val) {}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarBool ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarBool::VarBool(ModuleLoc loc, bool val) : Var(loc, VarInfo::BASIC), val(val) {}
bool VarBool::onSet(VirtualMachine &vm, Var *from)
{
    val = as<VarBool>(from)->getVal();
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarInt ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarInt::VarInt(ModuleLoc loc, int64_t _val) : Var(loc, VarInfo::BASIC), val(_val) {}
VarInt::VarInt(ModuleLoc loc, const char *_val) : Var(loc, VarInfo::BASIC), val(std::stoll(_val)) {}
bool VarInt::onSet(VirtualMachine &vm, Var *from)
{
    val = as<VarInt>(from)->getVal();
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// VarIntIterator ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarIntIterator::VarIntIterator(ModuleLoc loc)
    : Var(loc, VarInfo::BASIC), started(false), reversed(false), begin(0), end(0), step(0), curr(0)
{}
VarIntIterator::VarIntIterator(ModuleLoc loc, int64_t _begin, int64_t _end, int64_t _step)
    : Var(loc, VarInfo::BASIC), started(false), reversed(_step < 0), begin(_begin), end(_end),
      step(_step), curr(_begin)
{}

bool VarIntIterator::next(int64_t &val)
{
    if(reversed) {
        if(curr <= end) return false;
    } else {
        if(curr >= end) return false;
    }
    if(!started) {
        val     = curr;
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

VarFlt::VarFlt(ModuleLoc loc, double _val) : Var(loc, VarInfo::BASIC), val(_val) {}
VarFlt::VarFlt(ModuleLoc loc, const char *_val) : Var(loc, VarInfo::BASIC), val(std::stold(_val)) {}
bool VarFlt::onSet(VirtualMachine &vm, Var *from)
{
    val = as<VarFlt>(from)->getVal();
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarStr ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarStr::VarStr(ModuleLoc loc, char val) : Var(loc, VarInfo::BASIC), val(1, val) {}
VarStr::VarStr(ModuleLoc loc, String &&val) : Var(loc, VarInfo::BASIC), val(std::move(val)) {}
VarStr::VarStr(ModuleLoc loc, StringRef val) : Var(loc, VarInfo::BASIC), val(val) {}
VarStr::VarStr(ModuleLoc loc, const char *val) : Var(loc, VarInfo::BASIC), val(val) {}
VarStr::VarStr(ModuleLoc loc, InitList<StringRef> _val) : Var(loc, VarInfo::BASIC)
{
    for(auto &e : _val) val += e;
}
VarStr::VarStr(ModuleLoc loc, const char *val, size_t count)
    : Var(loc, VarInfo::BASIC), val(val, count)
{}
bool VarStr::onSet(VirtualMachine &vm, Var *from)
{
    val = as<VarStr>(from)->getVal();
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarVec ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarVec::VarVec(ModuleLoc loc, size_t reservesz, bool asrefs) : Var(loc, 0), asrefs(asrefs)
{
    val.reserve(reservesz);
}
VarVec::VarVec(ModuleLoc loc, Vector<Var *> &&val, bool asrefs)
    : Var(loc, 0), val(std::move(val)), asrefs(asrefs)
{}
void VarVec::onDestroy(VirtualMachine &vm)
{
    for(auto it = val.rbegin(); it != val.rend(); ++it) vm.decVarRef(*it);
}
bool VarVec::onSet(VirtualMachine &vm, Var *from) { return setVal(vm, as<VarVec>(from)->getVal()); }
bool VarVec::setVal(VirtualMachine &vm, Span<Var *> newval)
{
    clear(vm);
    if(asrefs) {
        for(auto &v : newval) { vm.incVarRef(v); }
        val.assign(newval.begin(), newval.end());
    } else {
        for(auto &v : newval) {
            Var *cp = vm.copyVar(getLoc(), v);
            if(!cp) { return false; }
            push(vm, cp, false);
        }
    }
    return true;
}
void VarVec::clear(VirtualMachine &vm)
{
    for(auto &v : val) vm.decVarRef(v);
    val.clear();
}
void VarVec::setAt(VirtualMachine &vm, size_t idx, Var *data, bool iref)
{
    if(iref) vm.incVarRef(data);
    vm.decVarRef(val[idx]);
    val[idx] = data;
}
void VarVec::insert(VirtualMachine &vm, size_t idx, Var *data, bool iref)
{
    if(iref) vm.incVarRef(data);
    val.insert(val.begin() + idx, data);
}
void VarVec::erase(VirtualMachine &vm, size_t idx, bool dref)
{
    Var *data = val[idx];
    val.erase(val.begin() + idx);
    if(dref) vm.decVarRef(data);
}
void VarVec::push(VirtualMachine &vm, Var *data, bool iref)
{
    if(iref) vm.incVarRef(data);
    val.push_back(data);
}
void VarVec::pop(VirtualMachine &vm, bool dref)
{
    Var *data = val.back();
    if(dref) vm.decVarRef(data);
    val.pop_back();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// VarVecIterator ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarVecIterator::VarVecIterator(ModuleLoc loc, VarVec *vec)
    : Var(loc, VarInfo::BASIC), vec(vec), curr(0)
{}
void VarVecIterator::onCreate(VirtualMachine &vm) { vm.incVarRef(vec); }
void VarVecIterator::onDestroy(VirtualMachine &vm) { vm.decVarRef(vec); }

bool VarVecIterator::next(Var *&val)
{
    if(curr >= vec->getVal().size()) return false;
    val = vec->getVal()[curr++];
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarMap ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarMap::Iterator::Iterator(void *orderiter, StringMap<Var *>::iterator dataiter, bool forward)
    : orderiter(orderiter), dataiter(dataiter), forward(forward)
{}

VarMap::VarMap(ModuleLoc loc, bool ordered, bool asrefs)
    : Var(loc, VarInfo::ATTR_BASED), keyOrder(nullptr), ordered(ordered), asrefs(asrefs)
{}
void VarMap::onCreate(VirtualMachine &vm)
{
    MemoryManager &mem = vm.getMemoryManager();
    if(isOrdered()) keyOrder = mem.allocInit<ManagedRawList>(mem, "VarMap");
}
void VarMap::onDestroy(VirtualMachine &vm)
{
    clear(vm);
    MemoryManager &mem = vm.getMemoryManager();
    if(isOrdered()) mem.freeDeinit(keyOrder);
}
bool VarMap::onSet(VirtualMachine &vm, Var *from)
{
    return setVal(vm, as<VarMap>(from)->getVal(), as<VarMap>(from)->getOrder());
}
bool VarMap::setVal(VirtualMachine &vm, const StringMap<Var *> &newval, ManagedRawList *order)
{
    clear(vm);
    if(order) {
        char *it = nullptr;
        while((it = (char *)order->next(it))) {
            Var *tmp = newval.at(it);
            if(!asrefs) tmp = vm.copyVar(tmp->getLoc(), tmp);
            setAttr(vm, it, tmp, asrefs);
        }
    } else {
        for(auto &v : newval) {
            Var *tmp = v.second;
            if(asrefs) vm.incVarRef(tmp);
            else tmp = vm.copyVar(tmp->getLoc(), tmp);
            val.insert({v.first, tmp});
        }
    }
    return true;
}
void VarMap::clear(VirtualMachine &vm)
{
    if(isOrdered()) {
        char *it = nullptr;
        while((it = (char *)keyOrder->prev(it))) { vm.decVarRef(val[it]); }
        keyOrder->clear();
    } else {
        for(auto &v : val) vm.decVarRef(v.second);
    }
    val.clear();
}
void VarMap::setAttr(VirtualMachine &vm, StringRef name, Var *val, bool iref)
{
    if(iref) vm.incVarRef(val);
    auto loc = this->val.find(name);
    if(loc != this->val.end()) {
        vm.decVarRef(loc->second);
        loc->second = val;
        return;
    }
    if(isOrdered()) addOrder(name);
    this->val.insert({String(name), val});
    return;
}
void VarMap::remAttr(VirtualMachine &vm, StringRef name, bool &found, bool dref)
{
    auto loc = val.find(name);
    if(loc == val.end()) return;
    found = true;
    if(isOrdered()) remOrder(name);
    if(dref) vm.decVarRef(loc->second);
    val.erase(loc);
}
bool VarMap::existsAttr(StringRef name) { return this->val.find(name) != this->val.end(); }
Var *VarMap::getAttr(StringRef name)
{
    auto loc = this->val.find(name);
    if(loc == this->val.end()) return nullptr;
    return loc->second;
}
void VarMap::getAttrList(VirtualMachine &vm, VarVec *dest)
{
    if(isOrdered()) {
        char *it = nullptr;
        while((it = (char *)keyOrder->next(it))) {
            dest->push(vm, vm.makeVar<VarStr>(dest->getLoc(), it), true);
        }
        return;
    }
    for(auto &e : val) dest->push(vm, vm.makeVar<VarStr>(dest->getLoc(), e.first), true);
}

VarMap::Iterator VarMap::begin(bool forward)
{
    if(isOrdered()) {
        char *key = (char *)keyOrder->next();
        if(!key) return end();
        return Iterator(key, val.find(key), forward);
    }
    return Iterator(nullptr, val.begin(), true);
}

void VarMap::next(VarMap::Iterator &it)
{
    if(isOrdered()) {
        char *key = nullptr;
        if(it.isForward()) key = (char *)keyOrder->next(it.orderiter);
        else key = (char *)keyOrder->prev(it.orderiter);
        it.orderiter = key;
        it.dataiter  = key ? val.find(key) : val.end();
        return;
    }
    ++it.dataiter;
}

void VarMap::addOrder(StringRef name) { keyOrder->allocInit<char>(name.data(), name.size() + 1); }
void VarMap::remOrder(StringRef name)
{
    char *it = nullptr;
    while((it = (char *)keyOrder->next(it))) {
        if(name != it) continue;
        keyOrder->free(it);
        break;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// VarMapIterator ///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarMapIterator::VarMapIterator(ModuleLoc loc, VarMap *map)
    : Var(loc, VarInfo::BASIC), map(map), curr(map->begin())
{}
void VarMapIterator::onCreate(VirtualMachine &vm) { vm.incVarRef(map); }
void VarMapIterator::onDestroy(VirtualMachine &vm) { vm.decVarRef(map); }

bool VarMapIterator::next(VirtualMachine &vm, ModuleLoc loc, Var *&val)
{
    if(curr == map->end()) return false;
    StringMap<Var *> attrs;
    val = vm.makeVar<VarStruct>(loc, nullptr, typeID<VarMapIterator>());
    as<VarStruct>(val)->reserveAttrs(2);
    as<VarStruct>(val)->setAttr(vm, "0", vm.makeVar<VarStr>(loc, curr.key()), true);
    as<VarStruct>(val)->setAttr(vm, "1", vm.incVarRef(curr.val()), false);
    map->next(curr);
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarFn /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarFn::VarFn(ModuleLoc loc, VarModule *mod, Vector<String> &&params,
             StringMap<Var *> &&defaultParams, FnBody body, StringRef kwArg, StringRef vaArg,
             bool isnative, bool isvirtual)
    : Var(loc, VarInfo::BASIC | VarInfo::CALLABLE), mod(mod), params(std::move(params)),
      defaultParams(std::move(defaultParams)), body(body), kwArg(kwArg), vaArg(vaArg),
      isnative(isnative), isvirtual(isvirtual)
{}
void VarFn::onDestroy(VirtualMachine &vm)
{
    for(auto &a : defaultParams) vm.decVarRef(a.second);
}
Var *VarFn::onCall(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args, VarMap *assnArgs,
                   VarVec *stack, size_t *currentlyAt)
{
    if(args.size() < params.size() - defaultParams.size() ||
       (args.size() > params.size() && vaArg.empty()))
    {
        vm.fail(loc, "arg count must be within: [", params.size() - 1, ", ",
                params.size() - defaultParams.size() - 1, ")", "; received: ", args.size() - 1);
        return nullptr;
    }
    if(isNative()) {
        Var *res = body.native(vm, loc, args, assnArgs);
        if(!res) return nullptr;
        return vm.incVarRef(res);
    }

    VarStack *vars = vm.getVars();

    if(!isvirtual) {
        vm.pushModule(mod);
        vars->pushFn(vm, stack);
    }

    if(!currentlyAt || *currentlyAt == -1) {
        size_t i = 0;
        while(i < args.size() && i < params.size()) {
            if(args[i]) vars->setAttr(vm, params[i], args[i], true);
            ++i;
        }
        for(auto &defaultParam : defaultParams) {
            if(vars->getAttr(defaultParam.first)) continue;
            Var *cp = vm.copyVar(loc, defaultParam.second);
            if(!cp) return nullptr;
            vars->setAttr(vm, defaultParam.first, cp, false);
        }
        // add all remaining args to variadic args if possible
        if(!vaArg.empty()) {
            VarVec *v = vm.makeVar<VarVec>(loc, args.size() - i, false);
            while(i < args.size()) { v->push(vm, args[i++], true); }
            vars->setAttr(vm, vaArg, v, true);
        }
        if(!kwArg.empty()) {
            if(!assnArgs) return nullptr;
            vars->setAttr(vm, kwArg, assnArgs, true);
        }
    }

    Var *ret = nullptr;
    vm.execute(ret, currentlyAt, body.feral.begin, body.feral.end);

    if(!isvirtual) {
        vars->popFn(vm, stack);
        vm.popModule();
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// VarClosure //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarClosure::VarClosure(ModuleLoc loc, Var *callable)
    : Var(loc, VarInfo::CALLABLE), callable(callable), args(nullptr), assnArgs(nullptr)
{}

void VarClosure::onCreate(VirtualMachine &vm)
{
    vm.incVarRef(callable);
    args     = vm.incVarRef(vm.makeVar<VarVec>(getLoc(), 2, true));
    assnArgs = vm.incVarRef(vm.makeVar<VarMap>(getLoc(), true, true));
    args->push(vm, nullptr, false); // for self
}
void VarClosure::onDestroy(VirtualMachine &vm)
{
    vm.decVarRef(assnArgs);
    vm.decVarRef(args);
    vm.decVarRef(callable);
}

Var *VarClosure::onCall(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args, VarMap *assnArgs,
                        VarVec *stack, size_t *currentlyAt)
{
    bool argsStart = args.size() > 0 && args[0] == nullptr ? 1 : 0;
    for(size_t i = argsStart; i < args.size(); ++i) this->args->push(vm, args[i], true);
    if(assnArgs) {
        for(auto &aa : this->assnArgs->getVal()) {
            if(!assnArgs->existsAttr(aa.first)) assnArgs->setAttr(vm, aa.first, aa.second, true);
        }
    }
    Var *res = callable->call(vm, loc, this->args->getVal(), assnArgs ? assnArgs : this->assnArgs,
                              stack, currentlyAt);
    for(size_t i = argsStart; i < args.size(); ++i) this->args->pop(vm, true);
    return res;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// VarAsync ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarAsync::VarAsync(ModuleLoc loc, VarClosure *closure)
    : Var(loc, VarInfo::CALLABLE), closure(closure), stack(nullptr), currentlyAt(-1),
      returned(nullptr)
{}

void VarAsync::onCreate(VirtualMachine &vm)
{
    vm.incVarRef(closure);
    stack = vm.incVarRef(vm.makeVar<VarVec>(getLoc(), 0, false));
}
void VarAsync::onDestroy(VirtualMachine &vm)
{
    if(returned) vm.decVarRef(returned);
    vm.decVarRef(stack);
    vm.decVarRef(closure);
}

Var *VarAsync::onCall(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args, VarMap *assnArgs,
                      VarVec *stack, size_t *currentlyAt)
{
    if(this->currentlyAt != -1) vm.pushExecStack(args.size() > 1 ? args[1] : vm.getNil(), true);
    Var *res = closure->call(vm, loc, {}, nullptr, this->stack, &this->currentlyAt);
    if(this->currentlyAt == -1) returned = vm.incVarRef(res);
    return res;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// VarModule ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarModule::VarModule(ModuleLoc loc, StringRef path, Bytecode &&bc, ModuleId moduleId,
                     bool isVirtual)
    : Var(loc, VarInfo::BASIC | VarInfo::ATTR_BASED), path(path), bc(std::move(bc)),
      moduleId(moduleId), moduleFrame(nullptr), virtualMod(isVirtual)
{}
void VarModule::onCreate(VirtualMachine &vm)
{
    if(!virtualMod) moduleFrame = vm.incVarRef(vm.makeVar<VarFrame>(getLoc()));
}
void VarModule::onDestroy(VirtualMachine &vm)
{
    if(moduleFrame) vm.decVarRef(moduleFrame);
}
void VarModule::setAttr(VirtualMachine &vm, StringRef name, Var *val, bool iref)
{
    assert(moduleFrame);
    moduleFrame->setAttr(vm, name, val, iref);
}
bool VarModule::existsAttr(StringRef name)
{
    assert(moduleFrame);
    return moduleFrame->existsAttr(name);
}
Var *VarModule::getAttr(StringRef name)
{
    assert(moduleFrame);
    return moduleFrame->getAttr(name);
}
void VarModule::getAttrList(VirtualMachine &vm, VarVec *dest)
{
    assert(moduleFrame);
    return moduleFrame->getAttrList(vm, dest);
}
size_t VarModule::getAttrCount()
{
    assert(moduleFrame);
    return moduleFrame->getAttrCount();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// VarFrame ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarFrame::VarFrame(ModuleLoc loc) : Var(loc, VarInfo::BASIC) {}

void VarFrame::onCreate(VirtualMachine &vm)
{
    frame = vm.incVarRef(vm.makeVar<VarMap>(getLoc(), 0, false));
}
void VarFrame::onDestroy(VirtualMachine &vm) { vm.decVarRef(frame); }

void VarFrame::setAttr(VirtualMachine &vm, StringRef name, Var *val, bool iref)
{
    LockGuard<RecursiveMutex> _(mtx);
    return frame->setAttr(vm, name, val, iref);
}
void VarFrame::remAttr(VirtualMachine &vm, StringRef name, bool &found, bool dref)
{
    LockGuard<RecursiveMutex> _(mtx);
    return frame->remAttr(vm, name, found, dref);
}
bool VarFrame::existsAttr(StringRef name)
{
    LockGuard<RecursiveMutex> _(mtx);
    return frame->existsAttr(name);
}
Var *VarFrame::getAttr(StringRef name)
{
    LockGuard<RecursiveMutex> _(mtx);
    return frame->getAttr(name);
}
void VarFrame::getAttrList(VirtualMachine &vm, VarVec *dest)
{
    LockGuard<RecursiveMutex> _(mtx);
    return frame->getAttrList(vm, dest);
}
size_t VarFrame::getAttrCount()
{
    LockGuard<RecursiveMutex> _(mtx);
    return frame->getAttrCount();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarDll ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarDll::VarDll(ModuleLoc loc, DllInitFn initfn, DllDeinitFn deinitfn)
    : Var(loc, VarInfo::BASIC), initfn(initfn), deinitfn(deinitfn)
{}
void VarDll::onDestroy(VirtualMachine &vm)
{
    if(deinitfn) deinitfn(vm);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// VarStructDef ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarStructDef::VarStructDef(ModuleLoc loc)
    : Var(loc, VarInfo::CALLABLE | VarInfo::ATTR_BASED), attrs(nullptr), id(genStructEnumID())
{}
VarStructDef::VarStructDef(ModuleLoc loc, size_t id)
    : Var(loc, VarInfo::CALLABLE | VarInfo::ATTR_BASED), attrs(nullptr), id(id)
{}
void VarStructDef::onCreate(VirtualMachine &vm)
{
    attrs = vm.incVarRef(vm.makeVar<VarMap>(getLoc(), true, false));
}
void VarStructDef::onDestroy(VirtualMachine &vm) { vm.decVarRef(attrs); }

Var *VarStructDef::onCall(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args, VarMap *assnArgs,
                          VarVec *stack, size_t *currentlyAt)
{
    for(auto it = assnArgs->begin(); it != assnArgs->end(); assnArgs->next(it)) {
        if(!attrs->existsAttr(it.key())) {
            vm.fail(it.val()->getLoc(), "no attribute named '", it.key(),
                    "' in the structure definition");
            return nullptr;
        }
    }

    VarStruct *res = vm.incVarRef(vm.createVar<VarStruct>(loc, this, id));
    res->reserveAttrs(attrs->size());

    auto it = attrs->begin();
    for(auto argit = args.begin() + 1; argit != args.end(); ++argit) {
        auto &arg = *argit;
        if(it == attrs->end()) {
            vm.fail(arg->getLoc(), "provided more arguments than existing in structure definition");
            goto fail;
        }
        res->setAttr(vm, it.key(), vm.copyVar(loc, arg), false);
        attrs->next(it);
    }

    for(auto aa = assnArgs->begin(); aa != assnArgs->end(); assnArgs->next(aa)) {
        res->setAttr(vm, aa.key(), vm.copyVar(loc, aa.val()), false);
    }
    while(it != attrs->end()) {
        if(!res->existsAttr(it.key())) res->setAttr(vm, it.key(), vm.copyVar(loc, it.val()), false);
        attrs->next(it);
    }

    return vm.initVar(res);
fail:
    vm.decVarRef(res);
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VarStruct ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarStruct::VarStruct(ModuleLoc loc, VarStructDef *base)
    : Var(loc, VarInfo::ATTR_BASED), base(base), attrs(nullptr), id(genStructEnumID())
{}
VarStruct::VarStruct(ModuleLoc loc, VarStructDef *base, size_t id)
    : Var(loc, VarInfo::ATTR_BASED), base(base), attrs(nullptr), id(id)
{}
void VarStruct::onCreate(VirtualMachine &vm)
{
    if(base) vm.incVarRef(base);
    attrs = vm.incVarRef(vm.makeVar<VarMap>(getLoc(), false, false));
}
void VarStruct::onDestroy(VirtualMachine &vm)
{
    vm.decVarRef(attrs);
    if(base) vm.decVarRef(base);
}

bool VarStruct::onSet(VirtualMachine &vm, Var *from)
{
    VarStruct *st = as<VarStruct>(from);
    if(!vm.setVar(attrs, st->attrs)) return false;
    if(st->base) vm.incVarRef(st->base);
    if(base) vm.decVarRef(base);
    base = st->base;
    id   = st->id;
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// VarFailure //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarFailure::VarFailure(ModuleLoc loc, VarFn *handler, size_t popLoc, size_t recurseCount)
    : Var(loc, VarInfo::BASIC), handler(handler), popLoc(popLoc), recurseCount(recurseCount),
      handling(false)
{}

void VarFailure::onCreate(VirtualMachine &vm) { vm.incVarRef(handler); }
void VarFailure::onDestroy(VirtualMachine &vm) { vm.decVarRef(handler); }

Var *VarFailure::callHandler(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args)
{
    handling       = true;
    VarStack *vars = vm.getVars();
    vars->pushBlk(vm, loc, 1);
    Var *res = handler->call(vm, loc, args, nullptr);
    vars->popBlk(vm, 1);
    handling = false;
    reset();
    return res;
}

void VarFailure::reset()
{
    trace.clear();
    msg.clear();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// VarFile ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarPath::VarPath(ModuleLoc loc, StringRef init) : Var(loc, VarInfo::BASIC), val(init) {}
VarPath::VarPath(ModuleLoc loc, String &&init) : Var(loc, VarInfo::BASIC), val(std::move(init)) {}
VarPath::VarPath(ModuleLoc loc, Path init) : Var(loc, VarInfo::BASIC), val(std::move(init)) {}

void VarPath::onCreate(VirtualMachine &vm) { val = normal(); }
bool VarPath::onSet(VirtualMachine &vm, Var *from)
{
    val = as<VarPath>(from)->getVal();
    return true;
}

Path VarPath::normal()
{
    String tmp = val.generic_string();
    utils::stringReplace(tmp, "\\", "/");
    std::error_code ec;
    return Path(tmp).lexically_normal();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// VarFile ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarFile::VarFile(ModuleLoc loc, FILE *const file, const String &mode, const bool requiresClosing)
    : Var(loc, VarInfo::BASIC), file(file), mode(mode), requiresClosing(requiresClosing)
{}
void VarFile::onDestroy(VirtualMachine &vm)
{
    if(requiresClosing && file) fclose(file);
}

bool VarFile::onSet(VirtualMachine &vm, Var *from)
{
    if(requiresClosing) fclose(file);
    requiresClosing                    = as<VarFile>(from)->requiresClosing;
    file                               = as<VarFile>(from)->file;
    as<VarFile>(from)->requiresClosing = false;
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VarFileIterator //////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarFileIterator::VarFileIterator(ModuleLoc loc, VarFile *file)
    : Var(loc, VarInfo::BASIC), file(file)
{}
void VarFileIterator::onCreate(VirtualMachine &vm) { vm.incVarRef(file); }
void VarFileIterator::onDestroy(VirtualMachine &vm) { vm.decVarRef(file); }

bool VarFileIterator::next(VarStr *&val)
{
    if(!val) return false;
    char *lineptr   = NULL;
    size_t len      = 0;
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

VarBytebuffer::VarBytebuffer(ModuleLoc loc, size_t bufsz, size_t buflen, const char *buf)
    : Var(loc, VarInfo::BASIC), buffer(nullptr), bufsz(bufsz), buflen(buflen)
{
    if(bufsz > 0) buffer = (char *)malloc(bufsz);
    if(buflen > 0) memcpy(buffer, buf, buflen);
}
VarBytebuffer::~VarBytebuffer()
{
    if(bufsz > 0) free(buffer);
}

bool VarBytebuffer::onSet(VirtualMachine &vm, Var *from)
{
    VarBytebuffer *tmp = as<VarBytebuffer>(from);
    setData(tmp->buffer, tmp->buflen);
    return true;
}
void VarBytebuffer::setData(const char *newbuf, size_t newlen)
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

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// VarStack //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarStack::VarStack(ModuleLoc loc) : Var(loc, VarInfo::BASIC | VarInfo::ATTR_BASED) {}

void VarStack::onCreate(VirtualMachine &vm) {}
void VarStack::onDestroy(VirtualMachine &vm)
{
    for(auto it = stack.rbegin(); it != stack.rend(); ++it) { vm.decVarRef(*it); }
}

Var *VarStack::getAttr(StringRef name)
{
    assert(!stack.empty() && !modulePos.empty());
    size_t currModulePos = modulePos.back();
    size_t currFuncPos   = funcPos.empty() ? -1 : funcPos.back();
    Var *res             = nullptr;
    int64_t iterLimit    = currFuncPos == -1 ? currModulePos : currFuncPos;
    for(int64_t i = stack.size() - 1; i >= iterLimit; --i) {
        res = stack[i]->getAttr(name);
        if(res) break;
    }
    if(res == nullptr && currFuncPos != -1) { res = stack[currModulePos]->getAttr(name); }
    return res;
}

void VarStack::pushBlk(VirtualMachine &vm, ModuleLoc loc, size_t count)
{
    for(size_t i = 0; i < count; ++i) {
        VarFrame *blk = vm.incVarRef(vm.makeVar<VarFrame>(loc));
        stack.push_back(blk);
    }
}
void VarStack::popBlk(VirtualMachine &vm, size_t count)
{
    for(size_t i = 0; i < count; ++i) {
        vm.decVarRef(stack.back());
        stack.pop_back();
    }
}

void VarStack::pushMod(VirtualMachine &vm, VarModule *mod)
{
    modulePos.push_back(stack.size());
    stack.push_back(vm.incVarRef(mod->getVarFrame()));
}
void VarStack::popMod(VirtualMachine &vm)
{
    assert(!modulePos.empty());
    size_t currModulePos = modulePos.back();
    while(stack.size() > currModulePos) {
        vm.decVarRef(stack.back());
        stack.pop_back();
    }
    modulePos.pop_back();
}
void VarStack::pushFn(VirtualMachine &vm, VarVec *loadFrames)
{
    funcPos.push_back(stack.size());
    if(!loadFrames || loadFrames->empty()) {
        pushBlk(vm, {}, 1);
        return;
    }
    for(size_t i = 0; i < loadFrames->size(); ++i) {
        stack.push_back(vm.incVarRef(as<VarFrame>(loadFrames->at(i))));
    }
    loadFrames->clear(vm);
}
void VarStack::popFn(VirtualMachine &vm, VarVec *saveFrames)
{
    assert(!funcPos.empty());
    size_t currFuncPos = funcPos.back();
    while(stack.size() > currFuncPos) {
        if(saveFrames) saveFrames->insert(vm, 0, stack.back(), true);
        popBlk(vm, 1);
    }
    funcPos.pop_back();
}

void VarStack::pushLoop(VirtualMachine &vm, ModuleLoc loc)
{
    loopsFrom.push_back(stack.size());
    pushBlk(vm, loc, 1);
}
// 'break' also uses this
void VarStack::popLoop(VirtualMachine &vm)
{
    assert(loopsFrom.size() > 0 && "Cannot VarStack::popLoop() from an empty loop stack");
    if(stack.size() > loopsFrom.back()) popBlk(vm, stack.size() - loopsFrom.back());
    loopsFrom.pop_back();
}
void VarStack::continueLoop(VirtualMachine &vm)
{
    assert(loopsFrom.size() > 0 && "Cannot VarStack::popLoop() from an empty loop stack");
    if(stack.size() - 1 > loopsFrom.back()) popBlk(vm, stack.size() - 1 - loopsFrom.back());
}

} // namespace fer