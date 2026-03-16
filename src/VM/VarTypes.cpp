#include "VM/VarTypes.hpp"

#include "VM/VM.hpp"

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
Var *Var::call(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args, VarMap *assnArgs,
               VarStack *existingFnStack, size_t *currentlyAt)
{
    return onCall(vm, loc, args, assnArgs, existingFnStack, currentlyAt);
}

void Var::onCreate(VirtualMachine &vm) {}
void Var::onDestroy(VirtualMachine &vm) {}
bool Var::onSet(VirtualMachine &vm, Var *from) { return true; }
Var *Var::onCall(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args, VarMap *assnArgs,
                 VarStack *existingFnStack, size_t *currentlyAt)
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
        outStr += as<VarPath>(this)->getStr();
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

VarFn::VarFn(ModuleLoc loc, VarModule *mod, Vector<Var *> &&defaultArgs, size_t paramCount,
             size_t reqdRegisters, size_t argsStartRegister, FnBody body, bool withkw, bool withva,
             bool isnative)
    : Var(loc, VarInfo::BASIC | VarInfo::CALLABLE), mod(mod), defaultArgs(std::move(defaultArgs)),
      paramCount(paramCount), reqdRegisters(reqdRegisters), argsStartRegister(argsStartRegister),
      body(body), withkw(withkw), withva(withva), isnative(isnative)
{}
void VarFn::onDestroy(VirtualMachine &vm)
{
    for(auto &a : defaultArgs) vm.decVarRef(a);
}
Var *VarFn::onCall(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args, VarMap *assnArgs,
                   VarStack *existingFnStack, size_t *currentlyAt)
{
    if(args.size() < paramCount || (args.size() > paramCount + defaultArgs.size() && !withva)) {
        vm.fail(loc, "arg count must be within: [", paramCount - 1, ", ",
                paramCount + defaultArgs.size() - 1, ")", "; received: ", args.size() - 1);
        return nullptr;
    }
    if(isNative()) {
        Var *res = body.native(vm, loc, args, assnArgs);
        if(!res) return nullptr;
        return vm.incVarRef(res);
    }

    VarVars *vars     = vm.getVars();
    VarStack *fnstack = nullptr;
    if(existingFnStack) {
        fnstack = vm.incVarRef(existingFnStack);
        if(fnstack->getRegisterCount() == -1) fnstack->initRegisters(vm, loc, reqdRegisters);
    } else if(reqdRegisters > 0) {
        fnstack = vm.incVarRef(vm.createVar<VarStack>(loc));
        fnstack->initRegisters(vm, loc, reqdRegisters);
    } else {
        fnstack = vm.incVarRef(vars->getCurrFnStack());
    }
    if(fnstack == nullptr) {
        vm.fail(loc, "no func stack found to use");
        return nullptr;
    }

    if(args[0] != nullptr) fnstack->setAt(vm, 2, args[0], true);
    size_t index = argsStartRegister;
    size_t i     = 1;
    while(i < args.size() && i < paramCount + defaultArgs.size()) {
        fnstack->setAt(vm, index++, args[i++], true);
    }
    size_t defaultCtr = args.size() - paramCount;
    while(defaultCtr < defaultArgs.size()) {
        Var *cp = vm.copyVar(loc, defaultArgs[defaultCtr++]);
        if(!cp) return nullptr;
        fnstack->setAt(vm, index++, cp, false);
        ++i;
    }
    // add all remaining args to variadic args if possible
    if(withva) {
        VarVec *v = vm.makeVar<VarVec>(loc, args.size(), false);
        while(i < args.size()) { v->push(vm, args[i++], true); }
        fnstack->setAt(vm, 1, v, true);
    }
    if(withkw) {
        if(!assnArgs) return nullptr;
        fnstack->setAt(vm, 0, assnArgs, true);
    }
    Var *ret = nullptr;
    vm.pushModule(mod);
    vm.execute(ret, fnstack, currentlyAt, body.feral.begin, body.feral.end);
    vm.decVarRef(fnstack);
    vm.popModule();
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
                        VarStack *fnstack, size_t *currentlyAt)
{
    bool argsStart = args.size() > 0 && args[0] == nullptr ? 1 : 0;
    for(size_t i = argsStart; i < args.size(); ++i) this->args->push(vm, args[i], true);
    if(assnArgs) {
        for(auto &aa : this->assnArgs->getVal()) {
            if(!assnArgs->existsAttr(aa.first)) assnArgs->setAttr(vm, aa.first, aa.second, true);
        }
    }
    Var *res = callable->call(vm, loc, this->args->getVal(), assnArgs ? assnArgs : this->assnArgs,
                              fnstack, currentlyAt);
    for(size_t i = argsStart; i < args.size(); ++i) this->args->pop(vm, true);
    return res;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// VarAsync ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarAsync::VarAsync(ModuleLoc loc, VarClosure *closure)
    : Var(loc, VarInfo::CALLABLE), closure(closure), fnstack(nullptr), currentlyAt(-1),
      returned(nullptr)
{}

void VarAsync::onCreate(VirtualMachine &vm)
{
    vm.incVarRef(closure);
    fnstack = vm.incVarRef(vm.makeVar<VarStack>(getLoc()));
}
void VarAsync::onDestroy(VirtualMachine &vm)
{
    if(returned) vm.decVarRef(returned);
    vm.decVarRef(fnstack);
    vm.decVarRef(closure);
}

Var *VarAsync::onCall(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args, VarMap *assnArgs,
                      VarStack *existingFnStack, size_t *currentlyAt)
{
    if(this->currentlyAt != -1) { vm.pushExecStack(args.size() > 1 ? args[1] : vm.getNil(), true); }
    Var *res = closure->call(vm, loc, {}, nullptr, fnstack, &this->currentlyAt);
    if(this->currentlyAt == -1) returned = vm.incVarRef(res);
    return res;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// VarModule ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarModule::VarModule(ModuleLoc loc, StringRef path, Bytecode &&bc, ModuleId moduleId,
                     VarStack *varStack)
    : Var(loc, VarInfo::BASIC | VarInfo::ATTR_BASED), path(path), bc(std::move(bc)),
      moduleId(moduleId), varStack(varStack), ownsVars(varStack == nullptr)
{}
void VarModule::onCreate(VirtualMachine &vm)
{
    if(varStack == nullptr) varStack = vm.incVarRef(vm.makeVar<VarStack>(getLoc()));
}
void VarModule::onDestroy(VirtualMachine &vm)
{
    if(varStack && ownsVars) vm.decVarRef(varStack);
}
void VarModule::setAttr(VirtualMachine &vm, StringRef name, Var *val, bool iref)
{
    varStack->setAttr(vm, name, val, iref);
}
bool VarModule::existsAttr(StringRef name) { return varStack->existsAttr(name); }
Var *VarModule::getAttr(StringRef name) { return varStack->getAttr(name); }
void VarModule::getAttrList(VirtualMachine &vm, VarVec *dest)
{
    VarMap *frame = varStack->getFrameAt(0);
    if(!frame) return;
    auto &f = frame->getVal();
    for(auto &e : f) dest->push(vm, vm.makeVar<VarStr>(dest->getLoc(), e.first), true);
}
size_t VarModule::getAttrCount()
{
    VarMap *frame = varStack->getFrameAt(0);
    if(!frame) return 0;
    return frame->size();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// VarStack ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarStack::VarStack(ModuleLoc loc) : Var(loc, VarInfo::BASIC), registers(nullptr) {}

void VarStack::onCreate(VirtualMachine &vm)
{
    stack = vm.incVarRef(vm.makeVar<VarVec>(getLoc(), 0, false));
    if(!stack) return;
    return pushStack(vm, getLoc(), 1);
}
void VarStack::onDestroy(VirtualMachine &vm)
{
    vm.decVarRef(registers);
    popStack(vm, 1);
    vm.decVarRef(stack);
}

void VarStack::setAttr(VirtualMachine &vm, StringRef name, Var *val, bool iref)
{
    LockGuard<RecursiveMutex> _(mtx);
    return stack->back()->setAttr(vm, name, val, iref);
}
void VarStack::remAttr(VirtualMachine &vm, StringRef name, bool &found, bool dref)
{
    LockGuard<RecursiveMutex> _(mtx);
    for(auto layer = stack->getVal().rbegin(); layer != stack->getVal().rend(); ++layer) {
        (*layer)->remAttr(vm, name, found, dref);
        if(!found) continue;
        return;
    }
}

Var *VarStack::getAttr(StringRef name)
{
    LockGuard<RecursiveMutex> _(mtx);
    for(auto layer = stack->getVal().rbegin(); layer != stack->getVal().rend(); ++layer) {
        Var *res = (*layer)->getAttr(name);
        if(res) return res;
    }
    return nullptr;
}

void VarStack::pushStack(VirtualMachine &vm, ModuleLoc loc, size_t count)
{
    LockGuard<RecursiveMutex> _(mtx);
    for(size_t i = 0; i < count; ++i) {
        VarMap *f = vm.makeVar<VarMap>(loc, true, false);
        if(!f) return;
        stack->push(vm, f, true);
    }
}
void VarStack::popStack(VirtualMachine &vm, size_t count)
{
    LockGuard<RecursiveMutex> _(mtx);
    for(size_t i = 0; i < count; ++i) { stack->pop(vm, true); }
}

void VarStack::pushLoop(VirtualMachine &vm, ModuleLoc loc)
{
    LockGuard<RecursiveMutex> _(mtx);
    loopsFrom.push_back(stack->size());
    pushStack(vm, loc, 1);
}
// 'break' also uses this
void VarStack::popLoop(VirtualMachine &vm)
{
    LockGuard<RecursiveMutex> _(mtx);
    assert(loopsFrom.size() > 0 && "Cannot VarStack::popLoop() from an empty loop stack");
    if(stack->size() >= loopsFrom.back()) popStack(vm, stack->size() - loopsFrom.back());
    loopsFrom.pop_back();
}
void VarStack::continueLoop(VirtualMachine &vm)
{
    LockGuard<RecursiveMutex> _(mtx);
    assert(loopsFrom.size() > 0 && "Cannot VarStack::popLoop() from an empty loop stack");
    if(stack->size() - 1 > loopsFrom.back()) popStack(vm, stack->size() - 1 - loopsFrom.back());
}

void VarStack::initRegisters(VirtualMachine &vm, ModuleLoc loc, size_t registerCount)
{
    LockGuard<RecursiveMutex> _(mtx);
    registers = vm.incVarRef(vm.makeVar<VarVec>(loc, registerCount, false));
    for(size_t i = 0; i < registerCount; ++i) { registers->push(vm, nullptr, false); }
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
                          VarStack *fnstack, size_t *currentlyAt)
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
    handling      = true;
    VarVars *vars = vm.getVars();
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
    String tmp{val};
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
////////////////////////////////////////// VarVars ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

VarVars::VarVars(ModuleLoc loc) : Var(loc, VarInfo::BASIC | VarInfo::ATTR_BASED), fnstack(-1) {}

void VarVars::onCreate(VirtualMachine &vm)
{
    modScopeStack = vm.incVarRef(vm.makeVar<VarVec>(getLoc(), 0, false));
}
void VarVars::onDestroy(VirtualMachine &vm) { vm.decVarRef(modScopeStack); }

Var *VarVars::getAttr(StringRef name)
{
    assert(fnstack != -1);
    Var *res = fnvars[fnstack]->getAttr(name);
    if(res == nullptr && fnstack != 0) { res = fnvars[0]->getAttr(name); }
    return res;
}

void VarVars::pushBlk(VirtualMachine &vm, ModuleLoc loc, size_t count)
{
    fnvars[fnstack]->pushStack(vm, loc, count);
}

void VarVars::pushModScope(VirtualMachine &vm, VarStack *modScope)
{
    modScopeStack->push(vm, modScope, true);
    fnvars[0] = modScope;
    if(fnstack == -1) fnstack = 0;
}
void VarVars::popModScope(VirtualMachine &vm)
{
    assert(modScopeStack->size() > 0);
    modScopeStack->pop(vm, true);
    if(modScopeStack->empty()) {
        fnvars[0] = nullptr;
        fnstack   = -1;
    } else {
        fnvars[0] = as<VarStack>(modScopeStack->back());
    }
}
void VarVars::pushFn(VirtualMachine &vm, VarStack *fn)
{
    ++fnstack;
    if(fnstack == 0) return;
    fnvars[fnstack] = vm.incVarRef(fn);
}
void VarVars::popFn(VirtualMachine &vm)
{
    if(fnstack == 0) return;
    auto loc = fnvars.find(fnstack);
    vm.decVarRef(loc->second);
    fnvars.erase(loc);
    --fnstack;
}

VarVars::ScopedModScope::ScopedModScope(VirtualMachine &vm, VarVars *vars, VarStack *modScope)
    : vars(vars), vm(vm)
{
    vars->pushModScope(vm, modScope);
}
VarVars::ScopedModScope::~ScopedModScope() { vars->popModScope(vm); }

} // namespace fer