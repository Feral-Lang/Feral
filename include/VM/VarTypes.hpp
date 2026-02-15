#pragma once

#include "Bytecode.hpp"

namespace fer
{

namespace VarInfo
{
enum
{
    // main attributes
    CALLABLE   = 1 << 1,
    ATTR_BASED = 1 << 2,

    // runtime attributes
    LOAD_AS_REF = 1 << 3,
    CONST       = 1 << 4,
    INITIALIZED = 1 << 5,
};
} // namespace VarInfo

struct AssnArgData;
class VirtualMachine;

class VarStr;
class VarVec;

typedef bool (*DllInitFn)(VirtualMachine &vm, ModuleLoc loc);
#define INIT_DLL(name) extern "C" bool Init##name(VirtualMachine &vm, ModuleLoc loc)
typedef void (*DllDeinitFn)(MemoryManager &mem);
#define DEINIT_DLL(name) extern "C" void Deinit##name(MemoryManager &mem)

class Var : public IAllocated
{
    ModuleLoc loc;
    Atomic<ssize_t> ref;
    // for VarInfo
    size_t info;

    VarStr *doc;

    friend class VirtualMachine;

    inline bool isLoadAsRef() const { return info & VarInfo::LOAD_AS_REF; }
    inline bool isInitialized() const { return info & VarInfo::INITIALIZED; }

    inline void iref() { ++ref; }
    inline ssize_t dref() { return --ref; }
    inline ssize_t getRef() const { return ref; }

    // Proxy functions to use the functions to be implemented by the Var's.
    void create(MemoryManager &mem);
    void destroy(MemoryManager &mem);
    // Copy this variable.
    // By default - if a custom `copy()` member function is not implemented,
    // it just increments ref and returns `this`.
    Var *copy(VirtualMachine &vm, ModuleLoc loc);
    // Set `this` variable using `from`, which is guaranteed to be of the same type as `this`.
    bool set(VirtualMachine &vm, Var *from);

    // Following functions are to be implemented by the Var's as needed.

    // Called by vm.makeVar*() after Var's constructor.
    // By default, it does nothing.
    virtual void onCreate(MemoryManager &mem);
    // Called by vm.unmakeVar() before Var's destructor.
    // By default, it does nothing.
    virtual void onDestroy(MemoryManager &mem);
    // Set value(s) in this variable using a different variable of the same type.
    // As such, no type checking is required to cast `from` to the class in which
    // this function is implemented.
    virtual bool onSet(VirtualMachine &vm, Var *from);
    // Perform a call using this variable.
    virtual Var *onCall(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                        const StringMap<AssnArgData> &assnArgs, bool addFunc, bool addBlk);

    // Generally should be called only by vm.decVarRef(), unless you are sure that var is not
    // being used elsewhere.
    template<typename T>
    static typename std::enable_if<std::is_base_of<Var, T>::value, void>::type
    unmakeVar(MemoryManager &mem, T *var)
    {
        var->destroy(mem);
        mem.freeDeinit(var);
    }

protected:
    Var(ModuleLoc loc, size_t infoFlags);
    // No need to override the destructor. Override onDestroy() instead.
    virtual ~Var();

public:
    Var *call(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
              const StringMap<AssnArgData> &assnArgs, bool addFunc = true, bool addBlk = false);
    virtual void setAttr(MemoryManager &mem, StringRef name, Var *val, bool iref);
    virtual bool existsAttr(StringRef name);
    virtual Var *getAttr(StringRef name);
    virtual void getAttrList(MemoryManager &mem, VarVec *dest);
    virtual size_t getAttrCount();
    virtual size_t getSubType();

    void setDoc(MemoryManager &mem, VarStr *newDoc);
    void dump(String &outStr, VirtualMachine *vm);

    template<typename T>
    typename std::enable_if<std::is_base_of<Var, T>::value, bool>::type is() const
    {
        return typeid(*this).hash_code() == typeid(T).hash_code();
    }
    template<typename T>
    typename std::enable_if<std::is_base_of<Var, T>::value, bool>::type isDerivedFrom()
    {
        return dynamic_cast<T *>(this) != 0;
    }

    inline void setLoc(ModuleLoc _loc) { loc = _loc; }

    inline VarStr *getDoc() { return doc; }
    inline bool hasDoc() const { return doc != nullptr; }
    inline ModuleLoc getLoc() const { return loc; }
    inline size_t getType() { return typeid(*this).hash_code(); }

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

    // used in native function calls
    // supposed to call the overloaded new operator in Var
    template<typename T, typename... Args>
    static typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type
    makeVar(MemoryManager &mem, Args &&...args)
    {
        T *res = new(mem.allocRaw(sizeof(T), alignof(T))) T(std::forward<Args>(args)...);
        res->create(mem);
        return res;
    }
    template<typename T>
    static typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type incVarRef(T *var)
    {
        if(var == nullptr) return nullptr;
        var->iref();
        return var;
    }
    template<typename T>
    static typename std::enable_if<std::is_base_of<Var, T>::value, T *>::type
    decVarRef(MemoryManager &mem, T *&var, bool del = true)
    {
        if(var == nullptr) return nullptr;
        if(var->dref() <= 0 && del) {
            unmakeVar(mem, var);
            var = nullptr;
        }
        return var;
    }
};

template<typename T> T *as(Var *data) { return static_cast<T *>(data); }

template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, size_t>::type typeID()
{
    return typeid(T).hash_code();
}

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

    using Iterator      = Vector<Var *>::iterator;
    using ConstIterator = Vector<Var *>::const_iterator;

    void onDestroy(MemoryManager &mem) override;
    bool onSet(VirtualMachine &vm, Var *from) override;

public:
    VarVec(ModuleLoc loc, size_t reservesz, bool asrefs);
    VarVec(ModuleLoc loc, Vector<Var *> &&val, bool asrefs);

    bool setVal(VirtualMachine &vm, Span<Var *> newval);
    void clear(MemoryManager &mem);
    Iterator insert(ConstIterator iter, Var *data, bool iref);
    Iterator erase(MemoryManager &mem, ConstIterator iter, Var *data, bool dref);

    inline Iterator insert(ConstIterator iter, Var *data) { return val.insert(iter, data); }
    inline Iterator erase(ConstIterator iter) { return val.erase(iter); }
    inline void push(Var *v, bool iref)
    {
        if(iref) incVarRef(v);
        val.push_back(v);
    }
    inline void pop(MemoryManager &mem, bool dref)
    {
        if(dref) decVarRef(mem, val.back());
        val.pop_back();
    }
    inline void swap(size_t a, size_t b) { std::iter_swap(val.begin() + a, val.begin() + b); }
    inline bool isEmpty() { return val.empty(); }
    inline bool isRefVec() { return asrefs; }

    inline Vector<Var *> &getVal() { return val; }
    inline Var *&at(size_t idx) { return val[idx]; }
    inline Var *&back() { return val.back(); }
    inline Var *&front() { return val.front(); }
    inline size_t size() { return val.size(); }
    inline size_t capacity() { return val.capacity(); }
    inline Iterator begin() { return val.begin(); }
    inline ConstIterator begin() const { return val.begin(); }
    inline Iterator end() { return val.end(); }
    inline ConstIterator end() const { return val.end(); }
};

class VarVecIterator : public Var
{
    VarVec *vec;
    size_t curr;

    void onCreate(MemoryManager &mem) override;
    void onDestroy(MemoryManager &mem) override;

public:
    VarVecIterator(ModuleLoc loc, VarVec *vec);

    bool next(Var *&val);
};

class VarMap : public Var
{
    StringMap<Var *> val;
    Vector<String> pos; // Only used by kwargs.
    bool asrefs;

    void onDestroy(MemoryManager &mem) override;
    bool onSet(VirtualMachine &vm, Var *from) override;

public:
    VarMap(ModuleLoc loc, size_t reservesz, bool asrefs);
    VarMap(ModuleLoc loc, StringMap<Var *> &&val, bool asrefs);

    bool setVal(VirtualMachine &vm, const StringMap<Var *> &newval);
    void clear(MemoryManager &mem);

    // not inline because Var is incomplete type
    void setAttr(MemoryManager &mem, StringRef name, Var *val, bool iref) override;
    bool existsAttr(StringRef name) override;
    Var *getAttr(StringRef name) override;
    void getAttrList(MemoryManager &mem, VarVec *dest) override;
    inline size_t getAttrCount() override { return val.size(); }

    inline size_t size() { return val.size(); }
    inline StringMap<Var *> &getVal() { return val; }
    inline void initializePos(size_t count) { pos = Vector<String>(count, ""); }
    // Make sure to initializePos() first.
    inline void setPos(size_t idx, StringRef data) { pos[idx] = data; }
    inline void insert(StringRef key, Var *value) { val.insert({String(key), value}); }
    inline Span<const String> getPositions() const { return pos; }
    inline bool isRefMap() { return asrefs; }
};

class VarMapIterator : public Var
{
    VarMap *map;
    StringMap<Var *>::iterator curr;

    void onCreate(MemoryManager &mem) override;
    void onDestroy(MemoryManager &mem) override;

public:
    VarMapIterator(ModuleLoc loc, VarMap *map);

    bool next(MemoryManager &mem, ModuleLoc loc, Var *&val);
};

// used in native function calls
struct AssnArgData
{
    size_t pos; // index of the arg
    Var *val;
};

typedef Var *(*NativeFn)(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                         const StringMap<AssnArgData> &assnArgs);

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

#define NATIVE_FUNC_SIGNATURE(name)                                       \
    Var *func_##name(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args, \
                     const StringMap<AssnArgData> &assnArgs)

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

class VarFn : public Var
{
    ModuleId moduleId;
    String kwArg;
    String varArg;
    Vector<String> params;
    StringMap<Var *> assnParams;
    FnBody body;
    bool isnative;

    void onDestroy(MemoryManager &mem) override;

    Var *onCall(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                const StringMap<AssnArgData> &assnArgs, bool addFunc, bool addBlk) override;

public:
    // args must be pushed to vector separately - this is done to reduce vector copies
    VarFn(ModuleLoc loc, ModuleId moduleId, const String &kwArg, const String &varArg,
          size_t paramCount, size_t assnParamsCount, FnBody body, bool isnative);

    inline void pushParam(const String &param) { params.push_back(param); }
    inline void setParams(Span<String> newparams)
    {
        params.assign(newparams.begin(), newparams.end());
    }
    inline void insertAssnParam(const String &key, Var *val) { assnParams.insert({key, val}); }
    inline void setAssnParams(const StringMap<Var *> &newmap) { assnParams = newmap; }

    inline ModuleId getModuleId() { return moduleId; }
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

class VarStack;
// A VarModule cannot be copied. It will always return self when a copy is attempted.
class VarModule : public Var
{
    String path;
    Bytecode bc;
    ModuleId moduleId;
    VarStack *varStack;
    bool ownsVars;

    void onCreate(MemoryManager &mem) override;
    void onDestroy(MemoryManager &mem) override;

public:
    VarModule(ModuleLoc loc, StringRef path, Bytecode &&bc, ModuleId moduleId,
              VarStack *varStack = nullptr);

    // not inline because Vars is incomplete type
    void setAttr(MemoryManager &mem, StringRef name, Var *val, bool iref) override;
    bool existsAttr(StringRef name) override;
    Var *getAttr(StringRef name) override;
    void getAttrList(MemoryManager &mem, VarVec *dest) override;
    size_t getAttrCount() override;

    void addNativeFn(VirtualMachine &vm, StringRef name, const FeralNativeFnDesc &fnObj);
    void addNativeFn(MemoryManager &mem, StringRef name, const FeralNativeFnDesc &fnObj);
    void addNativeVar(VirtualMachine &vm, StringRef name, StringRef doc, Var *val,
                      bool iref = true);
    void addNativeVar(MemoryManager &mem, StringRef name, StringRef doc, Var *val,
                      bool iref = true);

    inline StringRef getPath() { return path; }
    inline const Bytecode &getBytecode() { return bc; }
    inline ModuleId getModuleId() { return moduleId; }
    inline VarStack *getVarStack() { return varStack; }
};

class VarDll : public Var
{
    DllInitFn initfn;
    DllDeinitFn deinitfn;

    void onDestroy(MemoryManager &mem) override;

public:
    VarDll(ModuleLoc loc, DllInitFn initfn, DllDeinitFn deinitfn);
};

class VarStructDef : public Var
{
    StringMap<Var *> attrs;
    Vector<String> attrorder;
    // type id of struct (struct id) which will be used as typeID for struct objects
    size_t id;

    void onDestroy(MemoryManager &mem) override;

    // returns VarStruct
    Var *onCall(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                const StringMap<AssnArgData> &assnArgs, bool addFunc, bool addBlk) override;

public:
    VarStructDef(ModuleLoc loc, size_t attrscount);
    VarStructDef(ModuleLoc loc, size_t attrscount, size_t id);

    void setAttr(MemoryManager &mem, StringRef name, Var *val, bool iref) override;
    inline bool existsAttr(StringRef name) override { return attrs.find(name) != attrs.end(); }
    Var *getAttr(StringRef name) override;
    void getAttrList(MemoryManager &mem, VarVec *dest) override;
    inline size_t getAttrCount() override { return attrs.size(); }

    inline size_t getSubType() override { return id; }

    inline void pushAttrOrder(StringRef attr) { attrorder.emplace_back(attr); }
    inline void setAttrOrderAt(size_t idx, StringRef attr) { attrorder[idx] = attr; }
    inline void setAttrOrder(Span<StringRef> neworder)
    {
        attrorder.assign(neworder.begin(), neworder.end());
    }
    inline Span<String> getAttrOrder() { return attrorder; }
    inline StringRef getAttrOrderAt(size_t idx) { return attrorder[idx]; }
    inline size_t getID() { return id; }
};

class VarStruct : public Var
{
    StringMap<Var *> attrs;
    VarStructDef *base;
    size_t id;

    void onCreate(MemoryManager &mem) override;
    void onDestroy(MemoryManager &mem) override;
    bool onSet(VirtualMachine &vm, Var *from) override;

public:
    // base can be nullptr (as is the case for enums)
    VarStruct(ModuleLoc loc, VarStructDef *base, size_t attrscount);
    // base can be nullptr (as is the case for enums)
    VarStruct(ModuleLoc loc, VarStructDef *base, size_t attrscount, size_t id);

    void setAttr(MemoryManager &mem, StringRef name, Var *val, bool iref) override;

    inline bool existsAttr(StringRef name) override { return attrs.find(name) != attrs.end(); }
    Var *getAttr(StringRef name) override;
    void getAttrList(MemoryManager &mem, VarVec *dest) override;
    inline size_t getAttrCount() override { return attrs.size(); }

    inline size_t getSubType() override { return id; }

    inline const StringMap<Var *> &getAttrs() { return attrs; }
    inline VarStructDef *getBase() { return base; }
};

class VarFailure : public Var
{
    Vector<ModuleLoc> trace;
    String msg;
    VarFn *handler;
    size_t popLoc;
    size_t recurseCount;
    bool handling; // is this failure currently being handled

    void onDestroy(MemoryManager &mem) override;

    void reset();

public:
    VarFailure(ModuleLoc loc, VarFn *handler, size_t popLoc, size_t recurseCount,
               bool irefHandler = true);

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

class VarFile : public Var
{
    FILE *file;
    String mode;
    bool requiresClosing;

    void onDestroy(MemoryManager &mem) override;
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

    void onCreate(MemoryManager &mem) override;
    void onDestroy(MemoryManager &mem) override;

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

} // namespace fer