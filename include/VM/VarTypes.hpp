#pragma once

#include "Allocator.hpp"
#include "Core.hpp"
#include "Module.hpp"

namespace fer
{

enum class VarInfo
{
	CALLABLE    = 1 << 0,
	ATTR_BASED  = 1 << 1,
	LOAD_AS_REF = 1 << 2,
};

struct AssnArgData;
class Interpreter;

class Var : public IAllocated
{
	const ModuleLoc *loc;
	size_t ref;

	// for VarInfo
	size_t info;

	friend class Interpreter;

	inline void unsetLoadAsRef() { info &= ~(size_t)VarInfo::LOAD_AS_REF; }
	inline bool isLoadAsRef() const { return info & (size_t)VarInfo::LOAD_AS_REF; }

	inline void iref() { ++ref; }
	inline size_t dref() { return --ref; }
	inline size_t getRef() const { return ref; }

	// Proxy functions to use the functions to be implemented by the Var's.
	void create(Interpreter &vm);
	void destroy(Interpreter &vm);
	Var *copy(Interpreter &vm, const ModuleLoc *loc);
	void set(Interpreter &vm, Var *from);

	// Following functions are to be implemented by the Var's as needed.

	// Called by vm.makeVar*() after Var's constructor.
	// By default, it does nothing.
	virtual void onCreate(Interpreter &vm);
	// Called by vm.unmakeVar() before Var's destructor.
	// By default, it does nothing.
	virtual void onDestroy(Interpreter &vm);
	// Copy this variable.
	// By default (if not overriden), it just increments ref and returns `this`.
	virtual Var *onCopy(Interpreter &vm, const ModuleLoc *loc);
	// Set value(s) in this variable using a different variable of the same type.
	// As such, no type checking is required to cast `from` to the class in which
	// this function is implemented.
	virtual void onSet(Interpreter &vm, Var *from);

protected:
	Var(const ModuleLoc *loc, bool callable, bool attr_based);
	// No need to override the destructor. Override onDestroy() instead.
	virtual ~Var();

public:
	template<typename T>
	typename std::enable_if<std::is_base_of<Var, T>::value, bool>::type is() const
	{
		return typeid(*this).hash_code() == typeid(T).hash_code();
	}

	inline void setLoc(const ModuleLoc *_loc) { loc = _loc; }

	inline const ModuleLoc *getLoc() const { return loc; }
	inline size_t getType() { return typeid(*this).hash_code(); }
	virtual size_t getTypeFnID();

	inline bool isCallable() const { return info & (size_t)VarInfo::CALLABLE; }
	inline bool isAttrBased() const { return info & (size_t)VarInfo::ATTR_BASED; }

	inline void setLoadAsRef() { info |= (size_t)VarInfo::LOAD_AS_REF; }

	virtual Var *call(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
			  const StringMap<AssnArgData> &assn_args);
	virtual void setAttr(Interpreter &vm, StringRef name, Var *val, bool iref);
	virtual bool existsAttr(StringRef name);
	virtual Var *getAttr(StringRef name);
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
	VarAll(const ModuleLoc *loc);
};

class VarNil : public Var
{
public:
	VarNil(const ModuleLoc *loc);
};

class VarTypeID : public Var
{
	size_t val;

	Var *onCopy(Interpreter &vm, const ModuleLoc *loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	VarTypeID(const ModuleLoc *loc, size_t val);

	inline void setVal(size_t newval) { val = newval; }
	inline size_t getVal() { return val; }
};

class VarBool : public Var
{
	bool val;

	Var *onCopy(Interpreter &vm, const ModuleLoc *loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	VarBool(const ModuleLoc *loc, bool val);

	inline void setVal(bool newval) { val = newval; }
	inline bool getVal() { return val; }
};

class VarInt : public Var
{
	int64_t val;

	Var *onCopy(Interpreter &vm, const ModuleLoc *loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	VarInt(const ModuleLoc *loc, int64_t _val);
	VarInt(const ModuleLoc *loc, const char *_val);

	inline void setVal(int64_t newval) { val = newval; }
	inline int64_t getVal() { return val; }
};

class VarIntIterator : public Var
{
	int64_t begin, end, step, curr;
	bool started;
	bool reversed;

	Var *onCopy(Interpreter &vm, const ModuleLoc *loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	VarIntIterator(const ModuleLoc *loc);
	VarIntIterator(const ModuleLoc *loc, int64_t _begin, int64_t _end, int64_t _step);

	bool next(int64_t &val);

	inline void setReversed(int64_t step) { reversed = step < 0; }
	inline int64_t getBegin() { return begin; }
	inline int64_t getEnd() { return end; }
	inline int64_t getStep() { return step; }
	inline int64_t getCurr() { return curr; }
};

class VarFlt : public Var
{
	long double val;

	Var *onCopy(Interpreter &vm, const ModuleLoc *loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	VarFlt(const ModuleLoc *loc, long double _val);
	VarFlt(const ModuleLoc *loc, const char *_val);

	inline void setVal(long double newval) { val = newval; }
	inline long double getVal() { return val; }
};

class VarStr : public Var
{
	String val;

	Var *onCopy(Interpreter &vm, const ModuleLoc *loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	VarStr(const ModuleLoc *loc, char val);
	VarStr(const ModuleLoc *loc, String &&val);
	VarStr(const ModuleLoc *loc, StringRef val);
	VarStr(const ModuleLoc *loc, const char *val);
	VarStr(const ModuleLoc *loc, InitList<StringRef> _val);
	VarStr(const ModuleLoc *loc, const char *val, size_t count);

	inline void setVal(StringRef newval) { val = newval; }
	inline String &getVal() { return val; }
};

class VarVec : public Var
{
	Vector<Var *> val;
	bool asrefs;

	using Iterator	    = Vector<Var *>::iterator;
	using ConstIterator = Vector<Var *>::const_iterator;

	void onDestroy(Interpreter &vm) override;
	Var *onCopy(Interpreter &vm, const ModuleLoc *loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	VarVec(const ModuleLoc *loc, size_t reservesz, bool asrefs);
	VarVec(const ModuleLoc *loc, Vector<Var *> &&val, bool asrefs);

	void setVal(Interpreter &vm, Span<Var *> newval);

	inline Iterator insert(ConstIterator iter, Var *data) { return val.insert(iter, data); }
	inline Iterator erase(ConstIterator iter) { return val.erase(iter); }
	inline void push(Var *v) { val.push_back(v); }
	inline void pop() { val.pop_back(); }
	inline void clear() { val.clear(); }
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

	void onCreate(Interpreter &vm) override;
	void onDestroy(Interpreter &vm) override;
	Var *onCopy(Interpreter &vm, const ModuleLoc *loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	VarVecIterator(const ModuleLoc *loc, VarVec *vec);

	bool next(Var *&val);
};

class VarMap : public Var
{
	StringMap<Var *> val;
	Vector<String> pos; // Only used by kwargs.
	bool asrefs;

	void onDestroy(Interpreter &vm) override;
	Var *onCopy(Interpreter &vm, const ModuleLoc *loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	VarMap(const ModuleLoc *loc, size_t reservesz, bool asrefs);
	VarMap(const ModuleLoc *loc, StringMap<Var *> &&val, bool asrefs);

	void setVal(Interpreter &vm, const StringMap<Var *> &newval);
	void clear(Interpreter &vm);

	// not inline because Var is incomplete type
	void setAttr(Interpreter &vm, StringRef name, Var *val, bool iref) override;
	bool existsAttr(StringRef name) override;
	Var *getAttr(StringRef name) override;

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

	void onCreate(Interpreter &vm) override;
	void onDestroy(Interpreter &vm) override;
	Var *onCopy(Interpreter &vm, const ModuleLoc *loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	VarMapIterator(const ModuleLoc *loc, VarMap *map);

	bool next(Interpreter &vm, const ModuleLoc *loc, Var *&val);
};

// used in native function calls
struct AssnArgData
{
	size_t pos; // index of the arg
	Var *val;
};

typedef Var *(*NativeFn)(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
			 const StringMap<AssnArgData> &assn_args);

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
	StringRef modpath;
	String kw_arg;
	String var_arg;
	Vector<String> params;
	StringMap<Var *> assn_params;
	FnBody body;
	bool is_native;

	void onDestroy(Interpreter &vm) override;
	Var *onCopy(Interpreter &vm, const ModuleLoc *loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	// args must be pushed to vector separately - this is done to reduce vector copies
	VarFn(const ModuleLoc *loc, StringRef modpath, const String &kw_arg, const String &var_arg,
	      size_t paramcount, size_t assn_params_count, FnBody body, bool is_native);

	Var *call(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args) override;

	inline void pushParam(const String &param) { params.push_back(param); }
	inline void setParams(Span<String> newparams)
	{
		params.assign(newparams.begin(), newparams.end());
	}
	inline void insertAssnParam(const String &key, Var *val) { assn_params.insert({key, val}); }
	inline void setAssnParams(const StringMap<Var *> &newmap) { assn_params = newmap; }

	inline StringRef getModulePath() { return modpath; }
	inline StringRef getKwArg() { return kw_arg; }
	inline StringRef getVarArg() { return var_arg; }
	inline Vector<String> &getParams() { return params; }
	inline StringRef getParam(size_t idx) { return params[idx]; }
	inline StringMap<Var *> &getAssnParam() { return assn_params; }
	inline Var *getAssnParam(StringRef name)
	{
		auto loc = assn_params.find(name);
		return loc == assn_params.end() ? nullptr : loc->second;
	}
	inline NativeFn getNativeFn() { return body.native; }
	inline FeralFnBody getFeralFnBody() { return body.feral; }
	inline bool isNative() { return is_native; }
};

class Vars;
class VarModule : public Var
{
	Module *mod;
	Vars *vars;
	bool is_owner;
	bool is_thread_copy;

	void onCreate(Interpreter &vm) override;
	void onDestroy(Interpreter &vm) override;
	Var *onCopy(Interpreter &vm, const ModuleLoc *loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	VarModule(const ModuleLoc *loc, Module *mod, Vars *vars = nullptr, bool is_owner = true,
		  bool is_thread_copy = false);

	// not inline because Vars is incomplete type
	void setAttr(Interpreter &vm, StringRef name, Var *val, bool iref) override;
	bool existsAttr(StringRef name) override;
	Var *getAttr(StringRef name) override;

	void addNativeFn(Interpreter &vm, StringRef name, NativeFn body, size_t args = 0,
			 bool is_va = false);
	void addNativeVar(StringRef name, Var *val, bool iref = true, bool module_level = false);

	inline Module *getMod() { return mod; }
	inline Vars *getVars() { return vars; }
	inline bool isOwner() { return is_owner; }
	inline bool isThreadCopy() { return is_thread_copy; }
};

class VarStructDef : public Var
{
	StringMap<Var *> attrs;
	Vector<String> attrorder;
	// type id of struct (struct id) which will be used as typeID for struct objects
	size_t id;

	void onDestroy(Interpreter &vm) override;
	Var *onCopy(Interpreter &vm, const ModuleLoc *loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	VarStructDef(const ModuleLoc *loc, size_t attrscount);
	VarStructDef(const ModuleLoc *loc, size_t attrscount, size_t id);

	// returns VarStruct
	Var *call(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args) override;

	void setAttr(Interpreter &vm, StringRef name, Var *val, bool iref) override;
	inline bool existsAttr(StringRef name) override { return attrs.find(name) != attrs.end(); }
	Var *getAttr(StringRef name) override;

	inline void pushAttrOrder(StringRef attr) { attrorder.emplace_back(attr); }
	inline void setAttrOrderAt(size_t idx, StringRef attr) { attrorder[idx] = attr; }
	inline void setAttrOrder(Span<StringRef> neworder)
	{
		attrorder.assign(neworder.begin(), neworder.end());
	}
	inline Span<String> getAttrOrder() { return attrorder; }
	inline StringRef getAttrOrderAt(size_t idx) { return attrorder[idx]; }
	inline size_t getID() { return id; }
	inline size_t getAttrCount() { return attrs.size(); }
};

class VarStruct : public Var
{
	StringMap<Var *> attrs;
	VarStructDef *base;
	size_t id;

	void onCreate(Interpreter &vm) override;
	void onDestroy(Interpreter &vm) override;
	Var *onCopy(Interpreter &vm, const ModuleLoc *loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	// base can be nullptr (as is the case for enums)
	VarStruct(const ModuleLoc *loc, VarStructDef *base, size_t attrscount);
	// base can be nullptr (as is the case for enums)
	VarStruct(const ModuleLoc *loc, VarStructDef *base, size_t attrscount, size_t id);

	void setAttr(Interpreter &vm, StringRef name, Var *val, bool iref) override;

	inline size_t getTypeFnID() override { return id; }

	inline bool existsAttr(StringRef name) override { return attrs.find(name) != attrs.end(); }
	Var *getAttr(StringRef name) override;

	inline const StringMap<Var *> &getAttrs() { return attrs; }
	inline VarStructDef *getBase() { return base; }
	inline size_t getAttrCount() { return attrs.size(); }
};

class VarFile : public Var
{
	FILE *file;
	String mode;
	bool owner;

	void onDestroy(Interpreter &vm) override;
	Var *onCopy(Interpreter &vm, const ModuleLoc *loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	VarFile(const ModuleLoc *loc, FILE *const file, const String &mode,
		const bool owner = true);

	inline void setMode(StringRef newmode) { mode = newmode; }
	inline void setOwner(bool isowner) { owner = isowner; }

	inline FILE *&getFile() { return file; }
	inline StringRef getMode() { return mode; }
	inline bool isOwner() { return owner; }
};

class VarFileIterator : public Var
{
	VarFile *file;

	void onCreate(Interpreter &vm) override;
	void onDestroy(Interpreter &vm) override;
	Var *onCopy(Interpreter &vm, const ModuleLoc *loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	VarFileIterator(const ModuleLoc *loc, VarFile *file);

	bool next(VarStr *&val);
};

class VarBytebuffer : public Var
{
	char *buffer;
	size_t bufsz;
	size_t buflen;

	Var *onCopy(Interpreter &vm, const ModuleLoc *loc) override;
	void onSet(Interpreter &vm, Var *from) override;

public:
	VarBytebuffer(const ModuleLoc *loc, size_t bufsz, size_t buflen = 0, char *buf = nullptr);
	~VarBytebuffer();

	void setData(char *newbuf, size_t newlen);

	inline void setLen(size_t len) { buflen = len; }
	inline char *&getBuf() { return buffer; }
	inline size_t getLen() { return buflen; }
	inline size_t capacity() { return bufsz; }
};

} // namespace fer