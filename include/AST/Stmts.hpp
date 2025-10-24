#pragma once

#include <cassert>

#include "Lexer.hpp"
#include "VM/Bytecode.hpp"

namespace fer::ast
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
    FORIN,
    RET,
    CONTINUE,
    BREAK,
    DEFER,
};

class Stmt : public IAllocated
{
protected:
    ModuleLoc loc;
    Stmts stype;

public:
    Stmt(Stmts stmtType, ModuleLoc loc);
    virtual ~Stmt();

    virtual void disp(bool hasNext) = 0;

    const char *getStmtTypeCString() const;
    inline const Stmts &getStmtType() const { return stype; }
    inline StringRef getStmtTypeString() const { return getStmtTypeCString(); }

#define isStmtX(X, ENUMVAL) \
    inline bool is##X() { return stype == ENUMVAL; }
    isStmtX(Block, BLOCK);
    isStmtX(Simple, SIMPLE);
    isStmtX(Expr, EXPR);
    isStmtX(FnArgs, FNARGS);
    isStmtX(Var, VAR);
    isStmtX(FnSig, FNSIG);
    isStmtX(FnDef, FNDEF);
    isStmtX(VarDecl, VARDECL);
    isStmtX(Cond, COND);
    isStmtX(For, FOR);
    isStmtX(ForIn, FORIN);
    isStmtX(Return, RET);
    isStmtX(Continue, CONTINUE);
    isStmtX(Break, BREAK);
    isStmtX(Defer, DEFER);

    inline ModuleLoc getLoc() { return loc; }
};

template<typename T> T *as(Stmt *data) { return static_cast<T *>(data); }

template<typename T> Stmt **asStmt(T **data) { return (Stmt **)(data); }

class StmtBlock : public Stmt
{
    Vector<Stmt *> stmts;
    bool istop;
    // false = Disable unload after each expr/simple stmt in codegen.
    // Used for conditionals created using ternary (... ? ... : ...) operator.
    bool shouldunload;

public:
    StmtBlock(ModuleLoc loc, const Vector<Stmt *> &stmts, bool istop);
    ~StmtBlock();
    static StmtBlock *create(ManagedAllocator &allocator, ModuleLoc loc,
                             const Vector<Stmt *> &stmts, bool istop);

    void disp(bool hasNext);

    inline void setTop(bool _istop) { istop = _istop; }
    inline void setUnload(bool _shouldunload) { shouldunload = _shouldunload; }
    inline Vector<Stmt *> &getStmts() { return stmts; }
    inline bool isTop() const { return istop; }
    inline bool shouldUnload() const { return shouldunload; }
};

class StmtSimple : public Stmt
{
    lex::Lexeme val;

public:
    StmtSimple(ModuleLoc loc, const lex::Lexeme &val);
    ~StmtSimple();
    static StmtSimple *create(ManagedAllocator &allocator, ModuleLoc loc, const lex::Lexeme &val);

    void disp(bool hasNext);

    inline void updateLexDataStr(StringRef newdata) { val.setDataStr(newdata); }
    inline lex::Lexeme &getLexValue() { return val; }

    inline StringRef getLexDataStr() const { return val.getDataStr(); }
    inline long double getLexDataFlt() const { return val.getDataFlt(); }
    inline int64_t getLexDataInt() const { return val.getDataInt(); }
};

class StmtFnArgs : public Stmt
{
    Vector<Stmt *> args;
    Vector<bool> unpackVector; // works for variadic as well since variadic is a vector

public:
    StmtFnArgs(ModuleLoc loc, Vector<Stmt *> &&args, Vector<bool> &&unpackVector);
    ~StmtFnArgs();
    static StmtFnArgs *create(ManagedAllocator &allocator, ModuleLoc loc, Vector<Stmt *> &&args,
                              Vector<bool> &&unpackVector);

    void disp(bool hasNext);

    inline void setArg(size_t idx, Stmt *a) { args[idx] = a; }
    inline Vector<Stmt *> &getArgs() { return args; }
    inline Stmt *&getArg(size_t idx) { return args[idx]; }
    inline bool unpackArg(size_t idx) { return unpackVector[idx]; }
};

class StmtExpr : public Stmt
{
    Stmt *lhs;
    lex::Lexeme oper;
    Stmt *rhs;

public:
    StmtExpr(ModuleLoc loc, Stmt *lhs, const lex::Lexeme &oper, Stmt *rhs);
    ~StmtExpr();

    static StmtExpr *create(ManagedAllocator &allocator, ModuleLoc loc, Stmt *lhs,
                            const lex::Lexeme &oper, Stmt *rhs);

    void disp(bool hasNext);

    inline Stmt *&getLHS() { return lhs; }
    inline Stmt *&getRHS() { return rhs; }
    inline lex::Lexeme &getOper() { return oper; }
    inline lex::Tok &getOperTok() { return oper.getTok(); }
};

class StmtVar : public Stmt
{
    lex::Lexeme name; // can be STR in case of assn args
    Stmt *in;
    Stmt *val;  // expr or simple
    bool isarg; // fndef param / fncall arg or not

public:
    StmtVar(ModuleLoc loc, const lex::Lexeme &name, Stmt *in, Stmt *val, bool isarg);
    ~StmtVar();
    // at least one of type or val must be present
    static StmtVar *create(ManagedAllocator &allocator, ModuleLoc loc, const lex::Lexeme &name,
                           Stmt *in, Stmt *val, bool isarg);

    void disp(bool hasNext);

    inline void setVal(Stmt *newval) { val = newval; }

    inline lex::Lexeme &getName() { return name; }
    inline Stmt *&getVal() { return val; }
    inline Stmt *&getIn() { return in; }
    inline bool isArg() { return isarg; }
};

class StmtFnSig : public Stmt
{
    // StmtVar contains name and, optionally, val
    Vector<StmtVar *> args;
    StmtSimple *kwarg, *vaarg;

public:
    StmtFnSig(ModuleLoc loc, const Vector<StmtVar *> &args, StmtSimple *kwarg, StmtSimple *vaarg);
    ~StmtFnSig();
    static StmtFnSig *create(ManagedAllocator &allocator, ModuleLoc loc,
                             const Vector<StmtVar *> &args, StmtSimple *kwarg, StmtSimple *vaarg);

    void disp(bool hasNext);

    inline void insertArg(StmtVar *arg) { args.push_back(arg); }
    inline void insertArg(size_t pos, StmtVar *arg) { args.insert(args.begin() + pos, arg); }

    inline StmtVar *&getArg(size_t idx) { return args[idx]; }
    inline Vector<StmtVar *> &getArgs() { return args; }
    inline StmtSimple *&getKwArg() { return kwarg; }
    inline StmtSimple *&getVaArg() { return vaarg; }
};

class StmtFnDef : public Stmt
{
    StmtFnSig *sig;
    StmtBlock *blk;

public:
    StmtFnDef(ModuleLoc loc, StmtFnSig *sig, StmtBlock *blk);
    ~StmtFnDef();
    static StmtFnDef *create(ManagedAllocator &allocator, ModuleLoc loc, StmtFnSig *sig,
                             StmtBlock *blk);

    void disp(bool hasNext);

    inline void setBlk(StmtBlock *_blk) { blk = _blk; }

    inline StmtFnSig *&getSig() { return sig; }
    inline StmtBlock *&getBlk() { return blk; }

    inline StmtVar *&getSigArg(size_t idx) { return sig->getArg(idx); }
    inline const Vector<StmtVar *> &getSigArgs() const { return sig->getArgs(); }
    inline StmtSimple *&getKwArg() { return sig->getKwArg(); }
    inline StmtSimple *&getVaArg() { return sig->getVaArg(); }
};

class StmtVarDecl : public Stmt
{
    Vector<StmtVar *> decls;

public:
    StmtVarDecl(ModuleLoc loc, const Vector<StmtVar *> &decls);
    ~StmtVarDecl();

    static StmtVarDecl *create(ManagedAllocator &allocator, ModuleLoc loc,
                               const Vector<StmtVar *> &decls);

    void disp(bool hasNext);

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
    StmtCond(ModuleLoc loc, const Vector<Conditional> &conds);
    ~StmtCond();
    static StmtCond *create(ManagedAllocator &allocator, ModuleLoc loc,
                            const Vector<Conditional> &conds);

    void disp(bool hasNext);

    inline Vector<Conditional> &getConditionals() { return conds; }
    inline Stmt *&getCond(size_t idx) { return conds[idx].getCond(); }
    inline StmtBlock *&getBlk(size_t idx) { return conds[idx].getBlk(); }
};

class StmtFor : public Stmt
{
    Stmt *init; // either of StmtVarDecl or StmtExpr
    Stmt *cond;
    Stmt *incr;
    StmtBlock *blk;

public:
    StmtFor(ModuleLoc loc, Stmt *init, Stmt *cond, Stmt *incr, StmtBlock *blk);
    ~StmtFor();
    // init, cond, incr can be nullptr
    static StmtFor *create(ManagedAllocator &allocator, ModuleLoc loc, Stmt *init, Stmt *cond,
                           Stmt *incr, StmtBlock *blk);

    void disp(bool hasNext);

    inline Stmt *&getInit() { return init; }
    inline Stmt *&getCond() { return cond; }
    inline Stmt *&getIncr() { return incr; }
    inline StmtBlock *&getBlk() { return blk; }
};

class StmtForIn : public Stmt
{
    lex::Lexeme iter;
    Stmt *in;
    StmtBlock *blk;

public:
    StmtForIn(ModuleLoc loc, const lex::Lexeme &iter, Stmt *in, StmtBlock *blk);
    ~StmtForIn();
    // init, cond, incr can be nullptr
    static StmtForIn *create(ManagedAllocator &allocator, ModuleLoc loc, const lex::Lexeme &iter,
                             Stmt *in, StmtBlock *blk);

    void disp(bool hasNext);

    inline lex::Lexeme &getIter() { return iter; }
    inline Stmt *&getIn() { return in; }
    inline StmtBlock *&getBlk() { return blk; }
};

class StmtRet : public Stmt
{
    Stmt *val;

public:
    StmtRet(ModuleLoc loc, Stmt *val);
    ~StmtRet();
    static StmtRet *create(ManagedAllocator &allocator, ModuleLoc loc, Stmt *val);

    void disp(bool hasNext);

    inline Stmt *&getRetVal() { return val; }
};

class StmtContinue : public Stmt
{
public:
    StmtContinue(ModuleLoc loc);
    static StmtContinue *create(ManagedAllocator &allocator, ModuleLoc loc);

    void disp(bool hasNext);
};

class StmtBreak : public Stmt
{
public:
    StmtBreak(ModuleLoc loc);
    static StmtBreak *create(ManagedAllocator &allocator, ModuleLoc loc);

    void disp(bool hasNext);
};

class StmtDefer : public Stmt
{
    Stmt *val;

public:
    StmtDefer(ModuleLoc loc, Stmt *val);
    ~StmtDefer();
    static StmtDefer *create(ManagedAllocator &allocator, ModuleLoc loc, Stmt *val);

    void disp(bool hasNext);

    inline Stmt *&getDeferVal() { return val; }
};

} // namespace fer::ast