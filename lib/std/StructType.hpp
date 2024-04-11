#pragma once

#include "VM/Interpreter.hpp"

namespace fer
{

class VarStructDef : public Var
{
	StringMap<Var *> attrs;
	Vector<String> attrorder;
	// type id of struct (struct id) which will be used as typeID for struct objects
	size_t id;

public:
	VarStructDef(const ModuleLoc *loc, size_t attrscount, size_t id);
	~VarStructDef();

	Var *copy(const ModuleLoc *loc) override;
	void set(Var *from) override;

	// returns VarStruct
	Var *call(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		  const Map<String, AssnArgData> &assn_args) override;

	void setAttr(StringRef name, Var *val, bool iref) override;
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

public:
	// base can be nullptr (as is the case for enums)
	VarStruct(const ModuleLoc *loc, VarStructDef *base, size_t attrscount, size_t id);
	~VarStruct();

	size_t getTypeFnID() override;

	Var *copy(const ModuleLoc *loc) override;
	void set(Var *from) override;

	void setAttr(StringRef name, Var *val, bool iref) override;
	inline bool existsAttr(StringRef name) override { return attrs.find(name) != attrs.end(); }
	Var *getAttr(StringRef name) override;

	inline const StringMap<Var *> &getAttrs() { return attrs; }
	inline VarStructDef *getBase() { return base; }
	inline size_t getAttrCount() { return attrs.size(); }
};

} // namespace fer