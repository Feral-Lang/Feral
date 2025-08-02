#pragma once

#include "AST/Stmts.hpp"

namespace fer::ast
{

class Pass
{
protected:
	size_t passid;
	Allocator &allocator;

	// https://stackoverflow.com/questions/51332851/alternative-id-generators-for-types
	template<typename T> static inline std::uintptr_t passID()
	{
		// no need for static variable as function is inlined
		return reinterpret_cast<std::uintptr_t>(&passID<T>);
	}

public:
	Pass(const size_t &passid, Allocator &allocator);
	virtual ~Pass();

	template<typename T>
	static typename std::enable_if<std::is_base_of<Pass, T>::value, size_t>::type genPassID()
	{
		return (size_t)Pass::passID<T>();
	}

	template<typename T>
	typename std::enable_if<std::is_base_of<Pass, T>::value, bool>::type isPass() const
	{
		return passid == Pass::passID<T>();
	}

	inline bool visitTree(Stmt *&stmt) { return visit(stmt, &stmt); }

	virtual bool visit(Stmt *stmt, Stmt **source) = 0;

	virtual bool visit(StmtBlock *stmt, Stmt **source)    = 0;
	virtual bool visit(StmtSimple *stmt, Stmt **source)   = 0;
	virtual bool visit(StmtFnArgs *stmt, Stmt **source)   = 0;
	virtual bool visit(StmtExpr *stmt, Stmt **source)     = 0;
	virtual bool visit(StmtVar *stmt, Stmt **source)      = 0;
	virtual bool visit(StmtFnSig *stmt, Stmt **source)    = 0;
	virtual bool visit(StmtFnDef *stmt, Stmt **source)    = 0;
	virtual bool visit(StmtVarDecl *stmt, Stmt **source)  = 0;
	virtual bool visit(StmtCond *stmt, Stmt **source)     = 0;
	virtual bool visit(StmtFor *stmt, Stmt **source)      = 0;
	virtual bool visit(StmtForIn *stmt, Stmt **source)    = 0;
	virtual bool visit(StmtRet *stmt, Stmt **source)      = 0;
	virtual bool visit(StmtContinue *stmt, Stmt **source) = 0;
	virtual bool visit(StmtBreak *stmt, Stmt **source)    = 0;
	virtual bool visit(StmtDefer *stmt, Stmt **source)    = 0;

	inline const size_t &getPassID() { return passid; }
};

template<typename T> T *as(Pass *t) { return static_cast<T *>(t); }

class PassManager
{
	// Must be a vector to be able to use push_back()
	Vector<Pass *> passes;

public:
	PassManager();
	PassManager(const PassManager &pm) = delete;
	~PassManager();

	bool visit(Stmt *&ptree);

	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<Pass, T>::value, void>::type add(Args &&...args)
	{
		passes.push_back(new T(std::forward<Args>(args)...));
	}

	template<typename T>
	typename std::enable_if<std::is_base_of<Pass, T>::value, T>::type *getPass()
	{
		Pass *res = nullptr;
		for(auto &p : passes) {
			if(!p->isPass<T>()) continue;
			res = p;
			break;
		}
		return as<T>(res);
	}
};

} // namespace fer::ast