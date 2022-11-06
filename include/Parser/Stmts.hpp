#pragma once

#include <cassert>

#include "Lexer.hpp"

namespace fer
{

enum Stmts : uint8_t
{
	BLOCK,
	SIMPLE,
	EXPR,
	FNARGS,
	VAR,   // Var and Val (<var> = <value>)
	FNSIG, // function signature
	FNDEF,
	VARDECL, // <VAR>[, <VAR>]*
	COND,
	FOR,
	RET,
	CONTINUE,
	BREAK,
	DEFER,
};

class Stmt
{
protected:
	const ModuleLoc *loc;
	Stmts stype;

public:
	Stmt(Stmts stmt_type, const ModuleLoc *loc);
	virtual ~Stmt();

	virtual void disp(bool has_next) = 0;

	const char *getStmtTypeCString() const;
	inline const Stmts &getStmtType() const { return stype; }
	inline StringRef getStmtTypeString() const { return getStmtTypeCString(); }

#define isStmtX(X, ENUMVAL) \
	inline bool is##X() { return stype == ENUMVAL; }
	isStmtX(Block, BLOCK);
	isStmtX(Simple, SIMPLE);
	isStmtX(Expr, EXPR);
	isStmtX(FnCallInfo, FNARGS);
	isStmtX(Var, VAR);
	isStmtX(FnSig, FNSIG);
	isStmtX(FnDef, FNDEF);
	isStmtX(VarDecl, VARDECL);
	isStmtX(Cond, COND);
	isStmtX(For, FOR);
	isStmtX(Return, RET);
	isStmtX(Continue, CONTINUE);
	isStmtX(Break, BREAK);
	isStmtX(Defer, DEFER);

	inline const ModuleLoc *getLoc() { return loc; }
	inline Module *getMod() const { return loc->getMod(); }
};

template<typename T> T *as(Stmt *data) { return static_cast<T *>(data); }

template<typename T> Stmt **asStmt(T **data) { return (Stmt **)(data); }

class StmtBlock : public Stmt
{
	Vector<Stmt *> stmts;
	bool is_top;

public:
	StmtBlock(const ModuleLoc *loc, const Vector<Stmt *> &stmts, bool is_top);
	~StmtBlock();
	static StmtBlock *create(Context &c, const ModuleLoc *loc, const Vector<Stmt *> &stmts,
				 bool is_top);

	void disp(bool has_next);

	inline Vector<Stmt *> &getStmts() { return stmts; }
	inline bool isTop() const { return is_top; }
};

class StmtSimple : public Stmt
{
	lex::Lexeme val;

public:
	StmtSimple(const ModuleLoc *loc, const lex::Lexeme &val);
	~StmtSimple();
	static StmtSimple *create(Context &c, const ModuleLoc *loc, const lex::Lexeme &val);

	void disp(bool has_next);

	inline void updateLexDataStr(StringRef newdata) { val.setDataStr(newdata); }
	inline lex::Lexeme &getLexValue() { return val; }
};

class StmtFnArgs : public Stmt
{
	Vector<Stmt *> args;

public:
	StmtFnArgs(const ModuleLoc *loc, const Vector<Stmt *> &args);
	~StmtFnArgs();
	static StmtFnArgs *create(Context &c, const ModuleLoc *loc, const Vector<Stmt *> &args);

	void disp(bool has_next);

	inline void setArg(size_t idx, Stmt *a) { args[idx] = a; }
	inline Vector<Stmt *> &getArgs() { return args; }
	inline Stmt *getArg(size_t idx) { return args[idx]; }
};

class StmtExpr : public Stmt
{
	Stmt *lhs;
	lex::Lexeme oper;
	Stmt *rhs;
	StmtBlock *or_blk;
	lex::Lexeme or_blk_var;

public:
	StmtExpr(const ModuleLoc *loc, Stmt *lhs, const lex::Lexeme &oper, Stmt *rhs);
	~StmtExpr();
	// or_blk and or_blk_var can be set separately - nullptr/INVALID by default
	static StmtExpr *create(Context &c, const ModuleLoc *loc, Stmt *lhs,
				const lex::Lexeme &oper, Stmt *rhs);

	void disp(bool has_next);

	inline void setOr(StmtBlock *blk, const lex::Lexeme &blk_var)
	{
		or_blk	   = blk;
		or_blk_var = blk_var;
	}

	inline Stmt *&getLHS() { return lhs; }
	inline Stmt *&getRHS() { return rhs; }
	inline lex::Lexeme &getOper() { return oper; }
	inline StmtBlock *&getOrBlk() { return or_blk; }
	inline lex::Lexeme &getOrBlkVar() { return or_blk_var; }
};

class StmtVar : public Stmt
{
	lex::Lexeme name;
	StmtSimple *in;
	Stmt *val; // expr or simple
	bool is_const;

public:
	StmtVar(const ModuleLoc *loc, const lex::Lexeme &name, StmtSimple *in, Stmt *val,
		bool is_const);
	~StmtVar();
	// at least one of type or val must be present
	static StmtVar *create(Context &c, const ModuleLoc *loc, const lex::Lexeme &name,
			       StmtSimple *in, Stmt *val, bool is_const);

	void disp(bool has_next);

	inline void setVal(Stmt *val) { val = val; }

	inline lex::Lexeme &getName() { return name; }
	inline Stmt *&getVal() { return val; }
	inline StmtSimple *getIn() { return in; }
	inline bool isConst() { return is_const; }
};

class StmtFnSig : public Stmt
{
	// StmtVar contains constness, name, and optionally val
	Vector<StmtVar *> args;
	Vector<bool> argconsts;
	bool is_variadic;

public:
	StmtFnSig(const ModuleLoc *loc, const Vector<StmtVar *> &args, bool is_variadic);
	~StmtFnSig();
	static StmtFnSig *create(Context &c, const ModuleLoc *loc, const Vector<StmtVar *> &args,
				 bool is_variadic);

	void disp(bool has_next);

	inline void insertArg(StmtVar *arg) { args.push_back(arg); }
	inline void insertArg(size_t pos, StmtVar *arg) { args.insert(args.begin() + pos, arg); }
	inline void setVariadic(bool va) { is_variadic = va; }

	inline StmtVar *&getArg(size_t idx) { return args[idx]; }
	inline Vector<StmtVar *> &getArgs() { return args; }
	inline bool isVariadic() const { return is_variadic; }
};

class StmtFnDef : public Stmt
{
	StmtFnSig *sig;
	StmtBlock *blk;

public:
	StmtFnDef(const ModuleLoc *loc, StmtFnSig *sig, StmtBlock *blk);
	~StmtFnDef();
	static StmtFnDef *create(Context &c, const ModuleLoc *loc, StmtFnSig *sig, StmtBlock *blk);

	void disp(bool has_next);

	inline void setBlk(StmtBlock *_blk) { blk = _blk; }

	inline StmtFnSig *&getSig() { return sig; }
	inline StmtBlock *&getBlk() { return blk; }

	inline StmtVar *&getSigArg(size_t idx) { return sig->getArg(idx); }
	inline const Vector<StmtVar *> &getSigArgs() const { return sig->getArgs(); }
	inline bool isSigVariadic() const { return sig->isVariadic(); }
};

class StmtVarDecl : public Stmt
{
	Vector<StmtVar *> decls;

public:
	StmtVarDecl(const ModuleLoc *loc, const Vector<StmtVar *> &decls);
	~StmtVarDecl();

	static StmtVarDecl *create(Context &c, const ModuleLoc *loc,
				   const Vector<StmtVar *> &decls);

	void disp(bool has_next);

	inline Vector<StmtVar *> &getDecls() { return decls; }
};

class Conditional
{
	Stmt *cond; // can be nullptr (else)
	StmtBlock *blk;

public:
	Conditional(Stmt *cond, StmtBlock *blk);
	~Conditional();

	inline void reset()
	{
		cond = nullptr;
		blk  = nullptr;
	}

	inline Stmt *&getCond() { return cond; }
	inline StmtBlock *&getBlk() { return blk; }
	inline const Stmt *getCond() const { return cond; }
	inline const StmtBlock *getBlk() const { return blk; }
};

class StmtCond : public Stmt
{
	Vector<Conditional> conds;

public:
	StmtCond(const ModuleLoc *loc, const Vector<Conditional> &conds);
	~StmtCond();
	static StmtCond *create(Context &c, const ModuleLoc *loc, const Vector<Conditional> &conds);

	void disp(bool has_next);

	inline Vector<Conditional> &getConditionals() { return conds; }
};

class StmtFor : public Stmt
{
	Stmt *init; // either of StmtVarDecl or StmtExpr
	Stmt *cond;
	Stmt *incr;
	StmtBlock *blk;

public:
	StmtFor(const ModuleLoc *loc, Stmt *init, Stmt *cond, Stmt *incr, StmtBlock *blk);
	~StmtFor();
	// init, cond, incr can be nullptr
	static StmtFor *create(Context &c, const ModuleLoc *loc, Stmt *init, Stmt *cond, Stmt *incr,
			       StmtBlock *blk);

	void disp(bool has_next);

	inline Stmt *&getInit() { return init; }
	inline Stmt *&getCond() { return cond; }
	inline Stmt *&getIncr() { return incr; }
	inline StmtBlock *&getBlk() { return blk; }
};

class StmtRet : public Stmt
{
	Stmt *val;

public:
	StmtRet(const ModuleLoc *loc, Stmt *val);
	~StmtRet();
	static StmtRet *create(Context &c, const ModuleLoc *loc, Stmt *val);

	void disp(bool has_next);

	inline Stmt *&getRetVal() { return val; }
};

class StmtContinue : public Stmt
{
public:
	StmtContinue(const ModuleLoc *loc);
	static StmtContinue *create(Context &c, const ModuleLoc *loc);

	void disp(bool has_next);
};

class StmtBreak : public Stmt
{
public:
	StmtBreak(const ModuleLoc *loc);
	static StmtBreak *create(Context &c, const ModuleLoc *loc);

	void disp(bool has_next);
};

class StmtDefer : public Stmt
{
	Stmt *val;

public:
	StmtDefer(const ModuleLoc *loc, Stmt *val);
	~StmtDefer();
	static StmtDefer *create(Context &c, const ModuleLoc *loc, Stmt *val);

	void disp(bool has_next);

	inline Stmt *&getDeferVal() { return val; }
};

} // namespace fer