#pragma once

#include "Bytecode.hpp"

namespace fer
{

namespace VarInfo
{
enum
{
    // main attributes
    BASIC      = 1 << 0, // no _init_/_deinit_ functions
    CALLABLE   = 1 << 1,
    ATTR_BASED = 1 << 2,

    // runtime attributes
    LOAD_AS_REF = 1 << 3,
    CONST       = 1 << 4,
    CREATED     = 1 << 5,
    INITIALIZED = 1 << 6,
};
} // namespace VarInfo

struct AssnArgData;
class VirtualMachine;

class Var;
class VarStr;
class VarVec;
class VarMap;

template<typename T> concept VarDerived = std::is_base_of_v<Var, T>;

typedef bool (*DllInitFn)(VirtualMachine &vm, ModuleLoc loc);
#define INIT_DLL(name) extern "C" bool Init##name(VirtualMachine &vm, ModuleLoc loc)
typedef void (*DllDeinitFn)(VirtualMachine &vm);
#define DEINIT_DLL(name) extern "C" void Deinit##name(VirtualMachine &vm)

class Var : public IAllocated
{
    ModuleLoc loc;
    Atomic<ssize_t> ref;
    // for VarInfo
    size_t info;

    VarStr *doc;

    friend class VirtualMachine;

    inline bool isLoadAsRef() const { return info & VarInfo::LOAD_AS_REF; }
    inline bool isCreated() const { return info & VarInfo::CREATED; }
    inline bool isInitialized() const { return info & VarInfo::INITIALIZED; }

    inline void iref() { ++ref; }
    inline ssize_t dref() { return --ref; }
    inline ssize_t getRef() const { return ref; }

    // Proxy functions to use the functions to be implemented by the Var's.
    void create(VirtualMachine &vm);
    void destroy(VirtualMachine &vm);
    void init(VirtualMachine &vm);
    void deinit(VirtualMachine &vm);
    // Copy this variable.
    // By default - if a custom `copy()` member function is not implemented,
    // it just increments ref and returns `this`.
    Var *copy(VirtualMachine &vm, ModuleLoc loc);
    // Set `this` variable using `from`, which is guaranteed to be of the same type as `this`.
    bool set(VirtualMachine &vm, Var *from);

    // Following functions are to be implemented by the Var's as needed.

    // Called by vm.makeVar*() after Var's constructor.
    // By default, it does nothing.
    virtual void onCreate(VirtualMachine &vm);
    // Called by vm.unmakeVar() before Var's destructor.
    // By default, it does nothing.
    virtual void onDestroy(VirtualMachine &vm);
    // Set value(s) in this variable using a different variable of the same type.
    // As such, no type checking is required to cast `from` to the class in which
    // this function is implemented.
    virtual bool onSet(VirtualMachine &vm, Var *from);
    // Perform a call using this variable.
    virtual Var *onCall(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args, VarMap *assnArgs,
                        bool addFunc, bool addBlk);

protected:
    Var(ModuleLoc loc, size_t infoFlags);
    // No need to override the destructor. Override onDestroy() instead.
    virtual ~Var();

public:
    Var *call(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args, VarMap *assnArgs,
              bool addFunc = true, bool addBlk = false);
    virtual void setAttr(VirtualMachine &vm, StringRef name, Var *val, bool iref);
    virtual void remAttr(VirtualMachine &vm, StringRef name, bool &found, bool dref);
    virtual bool existsAttr(StringRef name);
    virtual Var *getAttr(StringRef name);
    virtual void getAttrList(VirtualMachine &vm, VarVec *dest);
    virtual size_t getAttrCount();
    virtual size_t getSubType();

    void setDoc(VirtualMachine &vm, VarStr *newDoc);
    void setDoc(VirtualMachine &vm, ModuleLoc loc, StringRef newDoc);

    void dump(String &outStr, VirtualMachine *vm);

    template<VarDerived T> bool is() const
    {
        return typeid(*this).hash_code() == typeid(T).hash_code();
    }
    template<VarDerived T> bool isDerivedFrom() { return dynamic_cast<T *>(this) != 0; }

    inline void setLoc(ModuleLoc _loc) { loc = _loc; }

    inline VarStr *getDoc() { return doc; }
    inline bool hasDoc() const { return doc != nullptr; }
    inline ModuleLoc getLoc() const { return loc; }
    inline size_t getType() { return typeid(*this).hash_code(); }

    inline bool isBasic() const { return info & VarInfo::BASIC; }
    inline bool isCallable() const { return info & VarInfo::CALLABLE; }
    inline bool isAttrBased() const { return info & VarInfo::ATTR_BASED; }

    inline bool isConst() const { return info & VarInfo::CONST; }

    inline void setLoadAsRef()
    {
        // any const marked variable cannot be marked with loadAsRef()
        if(!isConst()) info |= VarInfo::LOAD_AS_REF;
    }
    inline void unsetLoadAsRef() { info &= ~VarInfo::LOAD_AS_REF; }

    inline void setConst() { info |= VarInfo::CONST; }
    inline void unsetConst() { info &= ~VarInfo::CONST; }

    inline void setCreated() { info |= VarInfo::CREATED; }
    inline void unsetCreated() { info &= ~VarInfo::CREATED; }

    inline void setInitialized() { info |= VarInfo::INITIALIZED; }
    inline void unsetInitialized() { info &= ~VarInfo::INITIALIZED; }
};

template<VarDerived T> T *as(Var *data) { return static_cast<T *>(data); }

template<VarDerived T> size_t typeID() { return typeid(T).hash_code(); }

// dummy type to denote all other types
class VarAll : public Var
{
    // No copy() since VarAll contains no value and represents a value in itself.
    // Essentially, creating a new copy of it serves no purpose.

public:
    VarAll(ModuleLoc loc);
};

class VarNil : public Var
{
public:
    VarNil(ModuleLoc loc);
};

class VarTypeID : public Var
{
    size_t val;

public:
    VarTypeID(ModuleLoc loc, size_t val);

    inline void setVal(size_t newval) { val = newval; }
    inline size_t getVal() { return val; }
};

class VarBool : public Var
{
    bool val;

    bool onSet(VirtualMachine &vm, Var *from) override;

public:
    VarBool(ModuleLoc loc, bool val);

    inline void setVal(bool newval) { val = newval; }
    inline bool getVal() { return val; }
};

class VarInt : public Var
{
    int64_t val;

    bool onSet(VirtualMachine &vm, Var *from) override;

public:
    VarInt(ModuleLoc loc, int64_t _val);
    VarInt(ModuleLoc loc, const char *_val);

    inline void setVal(int64_t newval) { val = newval; }
    inline int64_t getVal() { return val; }
};

class VarIntIterator : public Var
{
    int64_t begin, end, step, curr;
    bool started;
    bool reversed;

public:
    VarIntIterator(ModuleLoc loc);
    VarIntIterator(ModuleLoc loc, int64_t _begin, int64_t _end, int64_t _step);

    bool next(int64_t &val);

    inline void setReversed(int64_t step) { reversed = step < 0; }
    inline int64_t getBegin() { return begin; }
    inline int64_t getEnd() { return end; }
    inline int64_t getStep() { return step; }
    inline int64_t getCurr() { return curr; }
};

class VarFlt : public Var
{
    double val;

    bool onSet(VirtualMachine &vm, Var *from) override;

public:
    VarFlt(ModuleLoc loc, double _val);
    VarFlt(ModuleLoc loc, const char *_val);

    inline void setVal(double newval) { val = newval; }
    inline double getVal() { return val; }
};

class VarStr : public Var
{
    String val;

    bool onSet(VirtualMachine &vm, Var *from) override;

public:
    VarStr(ModuleLoc loc, char val);
    VarStr(ModuleLoc loc, String &&val);
    VarStr(ModuleLoc loc, StringRef val);
    VarStr(ModuleLoc loc, const char *val);
    VarStr(ModuleLoc loc, InitList<StringRef> _val);
    VarStr(ModuleLoc loc, const char *val, size_t count);

    inline void setVal(StringRef newval) { val = newval; }
    inline String &getVal() { return val; }
};

class VarVec : public Var
{
    Vector<Var *> val;
    bool asrefs;

    void onDestroy(VirtualMachine &vm) override;
    bool onSet(VirtualMachine &vm, Var *from) override;

public:
    VarVec(ModuleLoc loc, size_t reservesz, bool asrefs);
    VarVec(ModuleLoc loc, Vector<Var *> &&val, bool asrefs);

    bool setVal(VirtualMachine &vm, Span<Var *> newval);
    void clear(VirtualMachine &vm);
    void insert(VirtualMachine &vm, size_t idx, Var *data, bool iref);
    void erase(VirtualMachine &vm, size_t idx, bool dref);
    void push(VirtualMachine &vm, Var *data, bool iref);
    void pop(VirtualMachine &vm, bool dref);

    inline void swap(size_t a, size_t b) { std::iter_swap(val.begin() + a, val.begin() + b); }
    inline bool empty() { return val.empty(); }
    inline bool isRefVec() { return asrefs; }

    inline Vector<Var *> &getVal() { return val; }
    inline Var *&at(size_t idx) { return val[idx]; }
    inline Var *&back() { return val.back(); }
    inline Var *&front() { return val.front(); }
    inline size_t size() { return val.size(); }
    inline size_t capacity() { return val.capacity(); }
};

class VarVecIterator : public Var
{
    VarVec *vec;
    size_t curr;

    void onCreate(VirtualMachine &vm) override;
    void onDestroy(VirtualMachine &vm) override;

public:
    VarVecIterator(ModuleLoc loc, VarVec *vec);

    bool next(Var *&val);
};

class VarMap : public Var
{
    StringMap<Var *> val;
    ManagedRawList *keyOrder; // list<str>
    bool ordered;
    bool asrefs;

    void onCreate(VirtualMachine &vm) override;
    void onDestroy(VirtualMachine &vm) override;
    bool onSet(VirtualMachine &vm, Var *from) override;

public:
    class Iterator
    {
        void *orderiter;
        StringMap<Var *>::iterator dataiter;

        friend class VarMap;

    public:
        Iterator(void *orderiter, StringMap<Var *>::iterator dataiter);

        inline bool operator==(const Iterator &other) const
        {
            return orderiter == other.orderiter && dataiter == other.dataiter;
        }
        inline bool operator!=(const Iterator &other) const { return !(*this == other); }

        inline StringRef key() { return dataiter->first; }
        inline Var *val() { return dataiter->second; }
    };

    VarMap(ModuleLoc loc, bool ordered, bool asrefs);

    bool setVal(VirtualMachine &vm, const StringMap<Var *> &newval, ManagedRawList *order);
    void clear(VirtualMachine &vm);

    // not inline because Var is incomplete type
    void setAttr(VirtualMachine &vm, StringRef name, Var *val, bool iref) override;
    void remAttr(VirtualMachine &vm, StringRef name, bool &found, bool dref) override;
    bool existsAttr(StringRef name) override;
    Var *getAttr(StringRef name) override;
    void getAttrList(VirtualMachine &vm, VarVec *dest) override;
    inline size_t getAttrCount() override { return val.size(); }

    Iterator begin();
    inline Iterator end() { return Iterator(nullptr, val.end()); }
    void next(Iterator &it);

    void addOrder(StringRef name);
    void remOrder(StringRef name);

    inline void reserve(size_t count) { val.reserve(count); }

    inline size_t size() { return val.size(); }
    inline StringMap<Var *> &getVal() { return val; }
    inline ManagedRawList *getOrder() { return keyOrder; }
    inline bool isOrdered() { return ordered; }
    inline bool isRefMap() { return asrefs; }
};

class VarMapIterator : public Var
{
    VarMap *map;
    VarMap::Iterator curr;

    void onCreate(VirtualMachine &vm) override;
    void onDestroy(VirtualMachine &vm) override;

public:
    VarMapIterator(ModuleLoc loc, VarMap *map);

    bool next(VirtualMachine &vm, ModuleLoc loc, Var *&val);
};

typedef Var *(*NativeFn)(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args, VarMap *assnArgs);

class FeralNativeFnDesc
{
public:
    StringRef doc;
    NativeFn fn;
    size_t argCount;
    bool isVariadic;

    constexpr FeralNativeFnDesc(StringRef doc, NativeFn fn, size_t argCount, bool isVariadic)
        : doc(doc), fn(fn), argCount(argCount), isVariadic(isVariadic)
    {}
};

#define NATIVE_FUNC_SIGNATURE(name) \
    Var *func_##name(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args, VarMap *assnArgs)

#define FERAL_FUNC_DECL(name, argCount, isVariadic, doc) \
    NATIVE_FUNC_SIGNATURE(name);                         \
    static constexpr FeralNativeFnDesc name(doc, func_##name, argCount, isVariadic);
#define FERAL_FUNC_DEF(name) NATIVE_FUNC_SIGNATURE(name)

#define FERAL_FUNC(name, argCount, isVariadic, doc)                                  \
    NATIVE_FUNC_SIGNATURE(name);                                                     \
    static constexpr FeralNativeFnDesc name(doc, func_##name, argCount, isVariadic); \
    NATIVE_FUNC_SIGNATURE(name)

struct FeralFnBody
{
    size_t begin;
    size_t end;
};

union FnBody
{
    NativeFn native;
    FeralFnBody feral;
};

class VarModule;

class VarFn : public Var
{
    VarModule *mod;
    String kwArg;
    String varArg;
    Vector<String> params;
    StringMap<Var *> assnParams;
    FnBody body;
    bool isnative;

    void onDestroy(VirtualMachine &vm) override;

    Var *onCall(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args, VarMap *assnArgs, bool addFunc,
                bool addBlk) override;

public:
    // args must be pushed to vector separately - this is done to reduce vector copies
    VarFn(ModuleLoc loc, VarModule *mod, const String &kwArg, const String &varArg,
          size_t paramCount, size_t assnParamsCount, FnBody body, bool isnative);

    inline void pushParam(const String &param) { params.push_back(param); }
    inline void setParams(Span<String> newparams)
    {
        params.assign(newparams.begin(), newparams.end());
    }
    inline void insertAssnParam(const String &key, Var *val) { assnParams.insert({key, val}); }
    inline void setAssnParams(const StringMap<Var *> &newmap) { assnParams = newmap; }

    inline VarModule *getModule() { return mod; }
    inline StringRef getKwArg() { return kwArg; }
    inline StringRef getVarArg() { return varArg; }
    inline Vector<String> &getParams() { return params; }
    inline StringRef getParam(size_t idx) { return params[idx]; }
    inline StringMap<Var *> &getAssnParam() { return assnParams; }
    inline Var *getAssnParam(StringRef name)
    {
        auto loc = assnParams.find(name);
        return loc == assnParams.end() ? nullptr : loc->second;
    }
    inline NativeFn getNativeFn() { return body.native; }
    inline FeralFnBody getFeralFnBody() { return body.feral; }
    inline bool isNative() { return isnative; }
};

class VarStack : public Var
{
    RecursiveMutex mtx;
    Vector<size_t> loopsFrom;
    // each VarFrame is a stack frame
    // Vector is not used here as VarFrame has to be stored as a pointer.
    // This is so because otherwise, on vector resize, it will cause the VarFrame object to
    // delete and reconstruct, therefore incorrectly calling the dref() calls
    VarVec *stack; // VarVec<VarMap>

    void onCreate(VirtualMachine &vm) override;
    void onDestroy(VirtualMachine &vm) override;

public:
    VarStack(ModuleLoc loc);

    void setAttr(VirtualMachine &vm, StringRef name, Var *val, bool iref) override;
    void remAttr(VirtualMachine &vm, StringRef name, bool &found, bool dref) override;
    inline bool existsAttr(StringRef name) override { return stack->back()->existsAttr(name); }
    Var *getAttr(StringRef name) override;

    // use this instead of exists() if the Var* retrieval is actually required
    Var *get(StringRef name);

    void pushStack(VirtualMachine &vm, ModuleLoc loc, size_t count);
    void popStack(VirtualMachine &vm, size_t count);

    void pushLoop(VirtualMachine &vm, ModuleLoc loc);
    // 'break' also uses this
    void popLoop(VirtualMachine &vm);
    void continueLoop(VirtualMachine &vm);

    inline void resizeTo(VirtualMachine &vm, size_t count)
    {
        if(stack->size() > count) return popStack(vm, stack->size() - count);
    }

    inline VarMap *getFrameAt(size_t index)
    {
        return index < stack->size() ? as<VarMap>(stack->at(index)) : nullptr;
    }

    inline size_t size() { return stack->size(); }
};

// A VarModule cannot be copied. It will always return self when a copy is attempted.
class VarModule : public Var
{
    String path;
    Bytecode bc;
    ModuleId moduleId;
    VarStack *varStack;
    bool ownsVars;

    void onCreate(VirtualMachine &vm) override;
    void onDestroy(VirtualMachine &vm) override;

public:
    VarModule(ModuleLoc loc, StringRef path, Bytecode &&bc, ModuleId moduleId,
              VarStack *varStack = nullptr);

    // not inline because Vars is incomplete type
    void setAttr(VirtualMachine &vm, StringRef name, Var *val, bool iref) override;
    bool existsAttr(StringRef name) override;
    Var *getAttr(StringRef name) override;
    void getAttrList(VirtualMachine &vm, VarVec *dest) override;
    size_t getAttrCount() override;

    inline StringRef getPath() { return path; }
    inline const Bytecode &getBytecode() { return bc; }
    inline ModuleId getModuleId() { return moduleId; }
    inline VarStack *getVarStack() { return varStack; }
};

class VarDll : public Var
{
    DllInitFn initfn;
    DllDeinitFn deinitfn;

    void onDestroy(VirtualMachine &vm) override;

public:
    VarDll(ModuleLoc loc, DllInitFn initfn, DllDeinitFn deinitfn);
};

class VarStructDef : public Var
{
    VarMap *attrs;
    // type id of struct (struct id) which will be used as typeID for struct objects
    size_t id;

    void onCreate(VirtualMachine &vm) override;
    void onDestroy(VirtualMachine &vm) override;

    // returns VarStruct
    Var *onCall(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args, VarMap *assnArgs, bool addFunc,
                bool addBlk) override;

public:
    VarStructDef(ModuleLoc loc);
    VarStructDef(ModuleLoc loc, size_t id);

    inline void setAttr(VirtualMachine &vm, StringRef name, Var *val, bool iref) override
    {
        return attrs->setAttr(vm, name, val, iref);
    }
    inline bool existsAttr(StringRef name) override { return attrs->existsAttr(name); }
    inline Var *getAttr(StringRef name) override { return attrs->getAttr(name); }
    inline void getAttrList(VirtualMachine &vm, VarVec *dest) override
    {
        return attrs->getAttrList(vm, dest);
    }
    inline size_t getAttrCount() override { return attrs->size(); }

    inline VarMap::Iterator attrBegin() { return attrs->begin(); }
    inline VarMap::Iterator attrEnd() { return attrs->end(); }
    inline void attrNext(VarMap::Iterator &it) { return attrs->next(it); }

    inline size_t getSubType() override { return id; }
    inline size_t getID() { return id; }

    inline void reserveAttrs(size_t count) { return attrs->reserve(count); }
};

class VarStruct : public Var
{
    VarStructDef *base;
    VarMap *attrs;
    size_t id;

    void onCreate(VirtualMachine &vm) override;
    void onDestroy(VirtualMachine &vm) override;
    bool onSet(VirtualMachine &vm, Var *from) override;

public:
    // base can be nullptr (as is the case for enums)
    VarStruct(ModuleLoc loc, VarStructDef *base);
    // base can be nullptr (as is the case for enums)
    VarStruct(ModuleLoc loc, VarStructDef *base, size_t id);

    inline void setAttr(VirtualMachine &vm, StringRef name, Var *val, bool iref) override
    {
        return attrs->setAttr(vm, name, val, iref);
    }
    inline bool existsAttr(StringRef name) override { return attrs->existsAttr(name); }
    inline Var *getAttr(StringRef name) override { return attrs->getAttr(name); }
    inline void getAttrList(VirtualMachine &vm, VarVec *dest) override
    {
        return attrs->getAttrList(vm, dest);
    }
    inline size_t getAttrCount() override { return attrs->size(); }

    inline VarMap::Iterator attrBegin() { return attrs->begin(); }
    inline VarMap::Iterator attrEnd() { return attrs->end(); }
    inline void attrNext(VarMap::Iterator &it) { return attrs->next(it); }

    inline size_t getSubType() override { return id; }
    inline VarStructDef *getBase() { return base; }

    inline void reserveAttrs(size_t count) { return attrs->reserve(count); }
};

class VarFailure : public Var
{
    Vector<ModuleLoc> trace;
    String msg;
    VarFn *handler;
    size_t popLoc;
    size_t recurseCount;
    bool handling; // is this failure currently being handled

    void onCreate(VirtualMachine &vm) override;
    void onDestroy(VirtualMachine &vm) override;

    void reset();

public:
    VarFailure(ModuleLoc loc, VarFn *handler, size_t popLoc, size_t recurseCount);

    Var *callHandler(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args);

    inline void setMsg(String &&newMsg)
    {
        using namespace std;
        swap(msg, newMsg);
    }

    inline void pushFrame(ModuleLoc loc)
    {
        if(trace.empty() || trace.back() != loc) trace.push_back(loc);
    }

    inline Span<ModuleLoc> getTrace() { return trace; }
    inline StringRef getMsg() { return msg; }
    inline bool hasMsg() { return !msg.empty(); }
    inline size_t getPopLoc() { return popLoc; }
    inline size_t getRecurseCount() { return recurseCount; }
    inline bool isHandling() { return handling; }
};

class VarPath : public Var
{
    Path val;

    void onCreate(VirtualMachine &vm) override;
    bool onSet(VirtualMachine &vm, Var *from) override;

public:
    VarPath(ModuleLoc loc, StringRef init = "");
    VarPath(ModuleLoc loc, String &&init);
    VarPath(ModuleLoc loc, Path init);

    Path normal();

    inline const String &getStr() { return val.native(); }

    inline Path join(StringRef path) { return val / path; }
    inline Path join(Path &path) { return val / path; }

    inline void append(StringRef path)
    {
        val /= path;
        val = normal();
    }
    inline void append(Path &path)
    {
        val /= path;
        val = normal();
    }

    inline bool hasRoot() { return val.has_root_path(); }
    inline bool hasRootName() { return val.has_root_name(); }
    inline bool hasRootDir() { return val.has_root_directory(); }
    inline bool hasRelative() { return val.has_relative_path(); }
    inline bool hasParent() { return val.has_parent_path(); }
    inline bool hasFile() { return val.has_filename(); }
    inline bool hasFileName() { return val.has_stem(); }
    inline bool hasFileExt() { return val.has_extension(); }

    inline Path root() { return val.root_path(); }
    inline Path rootName() { return val.root_name(); }
    inline Path rootDir() { return val.root_directory(); }
    inline Path absolute(std::error_code &ec) { return fs::canonical(val, ec); }
    inline Path relative() { return val.relative_path(); }
    inline Path relativeTo(const Path &path) { return val.lexically_relative(path); }
    inline Path parent() { return val.parent_path(); }
    inline Path file() { return val.filename(); }
    inline Path fileName() { return val.stem(); }
    inline Path fileExt() { return val.extension(); }

    inline void clear() { return val.clear(); }
    inline size_t size() { return val.native().size(); }
    inline bool empty() { return val.empty(); }

    inline Path &getVal() { return val; }
};

class VarFile : public Var
{
    FILE *file;
    String mode;
    bool requiresClosing;

    void onDestroy(VirtualMachine &vm) override;
    bool onSet(VirtualMachine &vm, Var *from) override;

public:
    VarFile(ModuleLoc loc, FILE *const file, const String &mode, bool requiresClosing = true);

    inline FILE *&getFile() { return file; }
    inline StringRef getMode() { return mode; }
    inline bool mustClose() { return requiresClosing; }
};

class VarFileIterator : public Var
{
    VarFile *file;

    void onCreate(VirtualMachine &vm) override;
    void onDestroy(VirtualMachine &vm) override;

public:
    VarFileIterator(ModuleLoc loc, VarFile *file);

    bool next(VarStr *&val);
};

class VarBytebuffer : public Var
{
    char *buffer;
    size_t bufsz;
    size_t buflen;

    bool onSet(VirtualMachine &vm, Var *from) override;

public:
    VarBytebuffer(ModuleLoc loc, size_t bufsz, size_t buflen = 0, const char *buf = nullptr);
    ~VarBytebuffer();

    void setData(const char *newbuf, size_t newlen);

    inline void setLen(size_t len) { buflen = len; }
    inline char *&getVal() { return buffer; }
    inline size_t size() { return buflen; }
    inline size_t capacity() { return bufsz; }
};

class VarVars : public Var
{
    // maps function ids to VarStack
    // 0 is the id for global (module) scope
    Map<size_t, VarStack *> fnvars;
    VarMap *stashed;
    VarVec *modScopeStack; // VarVec<VarStack>
    size_t fnstack;

    void onCreate(VirtualMachine &vm) override;
    void onDestroy(VirtualMachine &vm) override;

public:
    VarVars(ModuleLoc loc);

    inline void setAttr(VirtualMachine &vm, StringRef name, Var *val, bool iref) override
    {
        return fnvars[fnstack]->setAttr(vm, name, val, iref);
    }
    inline void remAttr(VirtualMachine &vm, StringRef name, bool &found, bool dref) override
    {
        return fnvars[fnstack]->remAttr(vm, name, found, dref);
    }
    // checks if variable exists in current scope ONLY
    inline bool existsAttr(StringRef name) override { return fnvars[fnstack]->existsAttr(name); }
    // use this instead of exists() if the Var* retrieval is actually required
    // and current scope requirement is not present
    Var *getAttr(StringRef name) override;

    void pushBlk(VirtualMachine &vm, ModuleLoc loc, size_t count);

    void pushModScope(VirtualMachine &vm, VarStack *modScope);
    void popModScope(VirtualMachine &vm);
    void pushFn(VirtualMachine &vm, ModuleLoc loc);
    void popFn(VirtualMachine &vm);
    void stash(VirtualMachine &vm, StringRef name, Var *val, bool iref = true);
    void unstash(VirtualMachine &vm);

    inline void popBlk(VirtualMachine &vm, size_t count)
    {
        return fnvars[fnstack]->popStack(vm, count);
    }
    inline size_t getBlkSize() { return fnvars[fnstack]->size(); }
    inline void resizeBlkTo(VirtualMachine &vm, size_t count)
    {
        return fnvars[fnstack]->resizeTo(vm, count);
    }

    inline VarStack *getCurrModScope()
    {
        return modScopeStack->empty() ? nullptr : as<VarStack>(modScopeStack->back());
    }

    inline void pushLoop(VirtualMachine &vm, ModuleLoc loc)
    {
        return fnvars[fnstack]->pushLoop(vm, loc);
    }
    inline void popLoop(VirtualMachine &vm) { return fnvars[fnstack]->popLoop(vm); }
    inline void continueLoop(VirtualMachine &vm) { return fnvars[fnstack]->continueLoop(vm); }

    class ScopedModScope
    {
        VarVars *vars;
        VirtualMachine &vm;

    public:
        ScopedModScope(VirtualMachine &vm, VarVars *vars, VarStack *modScope);
        ~ScopedModScope();
    };
};

} // namespace fer