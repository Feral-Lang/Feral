#pragma once

#include "Context.hpp"
#include "Parser/Stmts.hpp"

namespace fer
{

class ParserPass
{
protected:
	size_t passid;
	Context &ctx;

	// https://stackoverflow.com/questions/51332851/alternative-id-generators-for-types
	template<typename T> static inline std::uintptr_t passID()
	{
		// no need for static variable as function is inlined
		return reinterpret_cast<std::uintptr_t>(&passID<T>);
	}

public:
	ParserPass(const size_t &passid, Context &ctx);
	virtual ~ParserPass();

	template<typename T> static
	typename std::enable_if<std::is_base_of<ParserPass, T>::value, size_t>::type
	genPassID()
	{
		return (size_t)ParserPass::passID<T>();
	}

	template<typename T>
	typename std::enable_if<std::is_base_of<ParserPass, T>::value, bool>::type isPass() const
	{
		return passid == ParserPass::passID<T>();
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
	virtual bool visit(StmtRet *stmt, Stmt **source)      = 0;
	virtual bool visit(StmtContinue *stmt, Stmt **source) = 0;
	virtual bool visit(StmtBreak *stmt, Stmt **source)    = 0;
	virtual bool visit(StmtDefer *stmt, Stmt **source)    = 0;

	inline const size_t &getPassID() { return passid; }
};

template<typename T> T *as(ParserPass *t) { return static_cast<T *>(t); }

} // namespace fer