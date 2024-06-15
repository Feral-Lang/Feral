#pragma once

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

class Var
{
	const ModuleLoc *loc;
	size_t ref;

	// for VarInfo
	size_t info;

public:
	Var(const ModuleLoc *loc, bool callable, bool attr_based);
	virtual ~Var();

	template<typename T>
	typename std::enable_if<std::is_base_of<Var, T>::value, bool>::type is() const
	{
		return typeid(*this).hash_code() == typeid(T).hash_code();
	}

	inline void setLoc(const ModuleLoc *_loc) { loc = _loc; }

	inline const ModuleLoc *getLoc() const { return loc; }
	inline size_t getType() { return typeid(*this).hash_code(); }
	virtual size_t getTypeFnID();

	inline void iref() { ++ref; }
	inline size_t dref() { return --ref; }
	inline size_t getRef() const { return ref; }

	inline bool isCallable() const { return info & (size_t)VarInfo::CALLABLE; }
	inline bool isAttrBased() const { return info & (size_t)VarInfo::ATTR_BASED; }
	inline bool isLoadAsRef() const { return info & (size_t)VarInfo::LOAD_AS_REF; }

	inline void setLoadAsRef() { info |= (size_t)VarInfo::LOAD_AS_REF; }
	inline void unsetLoadAsRef() { info &= ~(size_t)VarInfo::LOAD_AS_REF; }

	virtual Var *copy(const ModuleLoc *loc) = 0;
	virtual void set(Var *from)		= 0;

	virtual Var *call(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
			  const StringMap<AssnArgData> &assn_args);
	virtual void setAttr(StringRef name, Var *val, bool iref);
	virtual bool existsAttr(StringRef name);
	virtual Var *getAttr(StringRef name);

	static void *operator new(size_t sz);
	static void operator delete(void *ptr, size_t sz);
};

template<typename T> T *as(Var *data) { return static_cast<T *>(data); }

template<typename T> typename std::enable_if<std::is_base_of<Var, T>::value, size_t>::type typeID()
{
	return typeid(T).hash_code();
}

template<typename T>
typename std::enable_if<std::is_base_of<Var, T>::value, void>::type inline incref(T *var)
{
	if(var == nullptr) return;
	var->iref();
}
template<typename T>
typename std::enable_if<std::is_base_of<Var, T>::value, void>::type inline decref(T *&var)
{
	if(var == nullptr) return;
	var->dref();
	if(var->getRef() == 0) {
		delete var;
		var = nullptr;
	}
}
// used in std/threads library
template<typename T>
typename std::enable_if<std::is_base_of<Var, T>::value, void>::type inline decrefConst(const T *var)
{
	if(var == nullptr) return;
	var->dref();
	if(var->ref() == 0) {
		delete var;
	}
}

// dummy type to denote all other types
class VarAll : public Var
{
public:
	VarAll(const ModuleLoc *loc);

	Var *copy(const ModuleLoc *loc) override;
	void set(Var *from) override;
};

class VarNil : public Var
{
public:
	VarNil(const ModuleLoc *loc);

	Var *copy(const ModuleLoc *loc) override;
	void set(Var *from) override;
};

class VarTypeID : public Var
{
	size_t val;

public:
	VarTypeID(const ModuleLoc *loc, size_t val);

	Var *copy(const ModuleLoc *loc) override;
	void set(Var *from) override;

	inline void set(size_t newval) { val = newval; }

	inline size_t get() { return val; }
};

class VarBool : public Var
{
	bool val;

public:
	VarBool(const ModuleLoc *loc, bool val);

	Var *copy(const ModuleLoc *loc) override;
	void set(Var *from) override;

	inline void set(bool newval) { val = newval; }

	inline bool get() { return val; }
};

class VarInt : public Var
{
	int64_t val;

public:
	VarInt(const ModuleLoc *loc, int64_t _val);
	VarInt(const ModuleLoc *loc, const char *_val);
	~VarInt();

	Var *copy(const ModuleLoc *loc) override;
	inline void set(Var *from) override;

	inline void set(int64_t newval) { val = newval; }
	inline int64_t get() { return val; }
};

class VarFlt : public Var
{
	long double val;

public:
	VarFlt(const ModuleLoc *loc, long double _val);
	VarFlt(const ModuleLoc *loc, const char *_val);
	~VarFlt();

	Var *copy(const ModuleLoc *loc) override;
	void set(Var *from) override;

	inline void set(long double newval) { val = newval; }
	inline long double get() { return val; }
};

class VarStr : public Var
{
	String val;

public:
	VarStr(const ModuleLoc *loc, char val);
	VarStr(const ModuleLoc *loc, String &&val);
	VarStr(const ModuleLoc *loc, StringRef val);
	VarStr(const ModuleLoc *loc, const char *val);
	VarStr(const ModuleLoc *loc, InitList<StringRef> _val);
	VarStr(const ModuleLoc *loc, const char *val, size_t count);

	Var *copy(const ModuleLoc *loc) override;
	void set(Var *from) override;

	inline void set(StringRef newval) { val = newval; }
	inline String &get() { return val; }
};

class VarVec : public Var
{
	Vector<Var *> val;
	bool asrefs;

	using Iterator	    = Vector<Var *>::iterator;
	using ConstIterator = Vector<Var *>::const_iterator;

public:
	VarVec(const ModuleLoc *loc, size_t reservesz, bool asrefs);
	VarVec(const ModuleLoc *loc, Vector<Var *> &&val, bool asrefs);
	~VarVec();

	Var *copy(const ModuleLoc *loc) override;
	void set(Span<Var *> newval);

	inline void set(Var *from) override { set(as<VarVec>(from)->get()); }

	inline Iterator insert(ConstIterator iter, Var *data) { return val.insert(iter, data); }
	inline Iterator erase(ConstIterator iter) { return val.erase(iter); }
	inline void push(Var *v) { val.push_back(v); }
	inline void pop() { val.pop_back(); }
	inline void clear() { val.clear(); }
	inline bool isEmpty() { return val.empty(); }
	inline bool isRefVec() { return asrefs; }

	inline Vector<Var *> &get() { return val; }
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

class VarMap : public Var
{
	StringMap<Var *> val;
	bool asrefs;

public:
	VarMap(const ModuleLoc *loc, size_t reservesz, bool asrefs);
	VarMap(const ModuleLoc *loc, StringMap<Var *> &&val, bool asrefs);
	~VarMap();

	Var *copy(const ModuleLoc *loc) override;
	inline void set(Var *from) override { set(as<VarMap>(from)->get()); }
	void set(const StringMap<Var *> &newval);

	inline StringMap<Var *> &get() { return val; }
	inline void insert(StringRef key, Var *value) { val.insert({String(key), value}); }
	inline bool isRefMap() { return asrefs; }
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

public:
	// args must be pushed to vector separately - this is done to reduce vector copies
	VarFn(const ModuleLoc *loc, StringRef modpath, const String &kw_arg, const String &var_arg,
	      size_t paramcount, size_t assn_params_count, FnBody body, bool is_native);
	~VarFn();

	Var *copy(const ModuleLoc *loc) override;
	void set(Var *from) override;
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

public:
	VarModule(const ModuleLoc *loc, Module *mod, Vars *vars = nullptr, bool is_owner = true,
		  bool is_thread_copy = false);
	~VarModule();

	Var *copy(const ModuleLoc *loc) override;
	Var *threadCopy(const ModuleLoc *loc);
	void set(Var *from) override;

	// not inline because Vars is incomplete type
	void setAttr(StringRef name, Var *val, bool iref) override;
	bool existsAttr(StringRef name) override;
	Var *getAttr(StringRef name) override;

	void addNativeFn(StringRef name, NativeFn body, size_t args = 0, bool is_va = false);
	void addNativeVar(StringRef name, Var *val, bool iref = true, bool module_level = false);

	inline Module *getMod() { return mod; }
	inline Vars *getVars() { return vars; }
	inline bool isOwner() { return is_owner; }
	inline bool isThreadCopy() { return is_thread_copy; }
};

} // namespace fer