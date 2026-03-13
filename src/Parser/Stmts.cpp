#include "AST/Stmts.hpp"

#include "TreeIO.hpp"

namespace fer::ast
{

///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// Stmt //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt::Stmt(Stmts stmtType, ModuleLoc loc) : loc(loc), stype(stmtType) {}
Stmt::~Stmt() {}

const char *Stmt::getStmtTypeCString() const
{
    switch(stype) {
    case BLOCK: return "block";
    case SIMPLE: return "simple";
    case EXPR: return "expression";
    case FNARGS: return "function args";
    case VAR: return "variable declaration base";
    case FNSIG: return "function signature";
    case FNDEF: return "function definition";
    case VARDECL: return "variable declaration";
    case COND: return "conditional";
    case FOR: return "for loop";
    case FORIN: return "forin loop";
    case RET: return "return";
    case CONTINUE: return "continue";
    case BREAK: return "break";
    case DEFER: return "defer";
    }
    return "";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtBlock ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtBlock::StmtBlock(ModuleLoc loc, Vector<Stmt *> &&stmts, bool istop)
    : Stmt(BLOCK, loc), stmts(std::move(stmts)), istop(istop), shouldunload(true)
{}
StmtBlock::~StmtBlock() {}
StmtBlock *StmtBlock::create(ManagedList &allocator, ModuleLoc loc, Vector<Stmt *> &&stmts,
                             bool istop)
{
    return allocator.alloc<StmtBlock>(loc, std::move(stmts), istop);
}

void StmtBlock::disp(bool hasNext)
{
    tio::taba(hasNext);
    tio::print(hasNext, {"Block [top = ", istop ? "yes" : "no",
                         "][should unload = ", shouldunload ? "yes" : "no", "]\n"});
    for(size_t i = 0; i < stmts.size(); ++i) {
        if(!stmts[i]) {
            tio::taba(hasNext);
            tio::print(i != stmts.size() - 1, {"<Source End>\n"});
            tio::tabr();
            continue;
        }
        stmts[i]->disp(i != stmts.size() - 1);
    }
    tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtSimple /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtSimple::StmtSimple(ModuleLoc loc, lex::TokType tokType, String &&val, size_t index)
    : Stmt(SIMPLE, loc), val(std::move(val)), index(index), tok(tokType)
{}
StmtSimple::StmtSimple(ModuleLoc loc, lex::TokType tokType, StringRef val, size_t index)
    : Stmt(SIMPLE, loc), val(String(val)), index(index), tok(tokType)
{}
StmtSimple::StmtSimple(ModuleLoc loc, lex::TokType tokType, int64_t val)
    : Stmt(SIMPLE, loc), val(val), index(-1), tok(tokType)
{}
StmtSimple::StmtSimple(ModuleLoc loc, lex::TokType tokType, double val)
    : Stmt(SIMPLE, loc), val(val), index(-1), tok(tokType)
{}

StmtSimple::~StmtSimple() {}
StmtSimple *StmtSimple::create(ManagedList &allocator, ModuleLoc loc, lex::TokType tokType,
                               String &&val, size_t index)
{
    return allocator.alloc<StmtSimple>(loc, tokType, std::move(val), index);
}
StmtSimple *StmtSimple::create(ManagedList &allocator, ModuleLoc loc, lex::TokType tokType,
                               StringRef val, size_t index)
{
    return allocator.alloc<StmtSimple>(loc, tokType, val, index);
}
StmtSimple *StmtSimple::create(ManagedList &allocator, ModuleLoc loc, lex::TokType tokType,
                               int64_t val)
{
    return allocator.alloc<StmtSimple>(loc, tokType, val);
}
StmtSimple *StmtSimple::create(ManagedList &allocator, ModuleLoc loc, lex::TokType tokType,
                               double val)
{
    return allocator.alloc<StmtSimple>(loc, tokType, val);
}

void StmtSimple::disp(bool hasNext)
{
    tio::taba(hasNext);
    lex::TokType ty = tok.getVal();
    String indexTmp = " (at index: " + std::to_string(index) + ")";
    if(hasDataStr()) {
        tio::print(hasNext,
                   {"Simple<", tok.cStr(), ">: ", getDataStr(), hasIndex() ? indexTmp : "", "\n"});
    } else if(hasDataInt()) {
        tio::print(hasNext, {"Simple<", tok.cStr(), ">: ", std::to_string(getDataInt()), "\n"});
    } else if(hasDataFlt()) {
        tio::print(hasNext, {"Simple<", tok.cStr(), ">: ", std::to_string(getDataFlt()), "\n"});
    }
    tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtFnArgs /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFnArgs::StmtFnArgs(ModuleLoc loc, Vector<Stmt *> &&args, Vector<bool> &&unpackVector)
    : Stmt(FNARGS, loc), args(args), unpackVector(unpackVector)
{}
StmtFnArgs::~StmtFnArgs() {}
StmtFnArgs *StmtFnArgs::create(ManagedList &allocator, ModuleLoc loc, Vector<Stmt *> &&args,
                               Vector<bool> &&unpackVector)
{
    return allocator.alloc<StmtFnArgs>(loc, std::move(args), std::move(unpackVector));
}

void StmtFnArgs::disp(bool hasNext)
{
    tio::taba(hasNext);
    tio::print(hasNext, {"Function Args: ", args.empty() ? "(empty)" : "", "\n"});
    if(!args.empty()) {
        for(size_t i = 0; i < args.size(); ++i) {
            tio::taba(i != args.size() - 1);
            tio::print(i != args.size() - 1,
                       {"Arg: [unpack = ", unpackArg(i) ? "true" : "false", "]\n"});
            args[i]->disp(false);
            tio::tabr();
        }
    }
    tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtExpr /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtExpr::StmtExpr(ModuleLoc loc, Stmt *lhs, lex::TokType oper, Stmt *rhs)
    : Stmt(EXPR, loc), lhs(lhs), oper(oper), rhs(rhs)
{}
StmtExpr::~StmtExpr() {}
StmtExpr *StmtExpr::create(ManagedList &allocator, ModuleLoc loc, Stmt *lhs, lex::TokType oper,
                           Stmt *rhs)
{
    return allocator.alloc<StmtExpr>(loc, lhs, oper, rhs);
}

void StmtExpr::disp(bool hasNext)
{
    tio::taba(hasNext);
    tio::print(hasNext, {"Expression\n"});
    if(lhs) {
        tio::taba(oper.isValid() || rhs);
        tio::print(oper.isValid() || rhs, {"LHS:\n"});
        lhs->disp(false);
        tio::tabr();
    }
    if(oper.isValid()) {
        tio::taba(rhs);
        tio::print(rhs, {"Oper: [", std::to_string(oper.getVal()), ":", oper.cStr(), "]\n"});
        tio::tabr();
    }
    if(rhs) {
        tio::taba(false);
        tio::print(false, {"RHS:\n"});
        rhs->disp(false);
        tio::tabr();
    }
    tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtVar //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtVar::StmtVar(ModuleLoc loc, StringRef name, Stmt *in, Stmt *val, size_t index, bool isarg)
    : Stmt(VAR, loc), name(name), doc(""), in(in), val(val), index(index), isarg(isarg)
{}
StmtVar::~StmtVar() {}
StmtVar *StmtVar::create(ManagedList &allocator, ModuleLoc loc, StringRef name, Stmt *in, Stmt *val,
                         size_t index, bool isarg)
{
    return allocator.alloc<StmtVar>(loc, name, in, val, index, isarg);
}

void StmtVar::disp(bool hasNext)
{
    tio::taba(hasNext);
    String indexTmp = " (at index: " + std::to_string(index) + ")";
    tio::print(hasNext,
               {isarg ? "Argument: " : "Variable: ", name, hasIndex() ? indexTmp : "", "\n"});
    if(hasDoc()) {
        tio::taba(in || val);
        tio::print(in || val, {"Doc: ", utils::toRawString(doc), "\n"});
        tio::tabr();
    }
    if(in) {
        tio::taba(val);
        tio::print(val, {"In:\n"});
        in->disp(val);
        tio::tabr();
    }
    if(val) {
        tio::taba(false);
        tio::print(false, {"Value:\n"});
        val->disp(false);
        tio::tabr();
    }
    tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFnSig ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFnSig::StmtFnSig(ModuleLoc loc, const Vector<StmtVar *> &args, StmtSimple *kwarg,
                     StmtSimple *vaarg, bool orblk)
    : Stmt(FNSIG, loc), args(args), kwarg(kwarg), vaarg(vaarg), orblk(orblk)
{}
StmtFnSig::~StmtFnSig() {}
StmtFnSig *StmtFnSig::create(ManagedList &allocator, ModuleLoc loc, const Vector<StmtVar *> &args,
                             StmtSimple *kwarg, StmtSimple *vaarg, bool orblk)
{
    return allocator.alloc<StmtFnSig>(loc, args, kwarg, vaarg, orblk);
}

void StmtFnSig::disp(bool hasNext)
{
    tio::taba(hasNext);
    tio::print(hasNext, {"Function Signature", isOrBlk() ? " for `or` block\n" : "\n"});
    if(kwarg) {
        tio::taba(true);
        tio::print(vaarg || args.size() > 0, {"Keyword Argument:\n"});
        kwarg->disp(vaarg || args.size() > 0);
        tio::tabr();
    }
    if(vaarg) {
        tio::taba(true);
        tio::print(args.size() > 0, {"Variadic Argument:\n"});
        vaarg->disp(args.size() > 0);
        tio::tabr();
    }
    if(args.size() > 0) {
        tio::taba(false);
        tio::print(false, {"Parameters\n"});
        for(size_t i = 0; i < args.size(); ++i) { args[i]->disp(i != args.size() - 1); }
        tio::tabr();
    }
    tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFnDef ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFnDef::StmtFnDef(ModuleLoc loc, StmtFnSig *sig, StmtBlock *blk, size_t reqdRegisters)
    : Stmt(FNDEF, loc), sig(sig), blk(blk), reqdRegisters(reqdRegisters)
{}
StmtFnDef::~StmtFnDef() {}
StmtFnDef *StmtFnDef::create(ManagedList &allocator, ModuleLoc loc, StmtFnSig *sig, StmtBlock *blk,
                             size_t reqdRegisters)
{
    return allocator.alloc<StmtFnDef>(loc, sig, blk, reqdRegisters);
}

void StmtFnDef::disp(bool hasNext)
{
    tio::taba(hasNext);
    String maxIndexTmp = " (required virtual registers: " + std::to_string(reqdRegisters) + ")";
    tio::print(hasNext,
               {"Function Definition", maxIndexTmp, isOrBlk() ? " for `or` block\n" : "\n"});
    sig->disp(true);
    blk->disp(false);
    tio::tabr(1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// StmtVarDecl /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtVarDecl::StmtVarDecl(ModuleLoc loc, const Vector<StmtVar *> &decls)
    : Stmt(VARDECL, loc), decls(decls)
{}
StmtVarDecl::~StmtVarDecl() {}
StmtVarDecl *StmtVarDecl::create(ManagedList &allocator, ModuleLoc loc,
                                 const Vector<StmtVar *> &decls)
{
    return allocator.alloc<StmtVarDecl>(loc, decls);
}

void StmtVarDecl::disp(bool hasNext)
{
    tio::taba(hasNext);
    tio::print(hasNext, {"Variable declarations\n"});
    for(size_t i = 0; i < decls.size(); ++i) { decls[i]->disp(i != decls.size() - 1); }
    tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtCond /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Conditional::Conditional(Stmt *cond, StmtBlock *blk) : cond(cond), blk(blk) {}
Conditional::~Conditional() {}

StmtCond::StmtCond(ModuleLoc loc, const Vector<Conditional> &conds) : Stmt(COND, loc), conds(conds)
{}
StmtCond::~StmtCond() {}
StmtCond *StmtCond::create(ManagedList &allocator, ModuleLoc loc, const Vector<Conditional> &conds)
{
    return allocator.alloc<StmtCond>(loc, conds);
}

void StmtCond::disp(bool hasNext)
{
    bool isInline = conds.size() > 0 && conds[0].getBlk() && conds[0].getBlk()->isTop();
    tio::taba(hasNext);
    tio::print(hasNext, {"Conditional [isInline: ", isInline ? "true" : "false", "]\n"});
    for(size_t i = 0; i < conds.size(); ++i) {
        tio::taba(i != conds.size() - 1);
        tio::print(i != conds.size() - 1, {"Branch:\n"});
        if(conds[i].getCond()) {
            tio::taba(true);
            tio::print(true, {"Condition:\n"});
            conds[i].getCond()->disp(false);
            tio::tabr();
        }
        tio::taba(false);
        tio::print(false, {"Block:\n"});
        conds[i].getBlk()->disp(false);
        tio::tabr(2);
    }
    tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFor //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFor::StmtFor(ModuleLoc loc, Stmt *init, Stmt *cond, Stmt *incr, StmtBlock *blk)
    : Stmt(FOR, loc), init(init), cond(cond), incr(incr), blk(blk)
{}
StmtFor::~StmtFor() {}
StmtFor *StmtFor::create(ManagedList &allocator, ModuleLoc loc, Stmt *init, Stmt *cond, Stmt *incr,
                         StmtBlock *blk)
{
    return allocator.alloc<StmtFor>(loc, init, cond, incr, blk);
}

void StmtFor::disp(bool hasNext)
{
    tio::taba(hasNext);
    tio::print(hasNext, {"For/While\n"});
    if(init) {
        tio::taba(cond || incr || blk);
        tio::print(cond || incr || blk, {"Init:\n"});
        init->disp(false);
        tio::tabr();
    }
    if(cond) {
        tio::taba(incr || blk);
        tio::print(incr || blk, {"Condition:\n"});
        cond->disp(false);
        tio::tabr();
    }
    if(incr) {
        tio::taba(blk);
        tio::print(blk, {"Increment:\n"});
        incr->disp(false);
        tio::tabr();
    }
    if(blk) {
        tio::taba(false);
        tio::print(false, {"Block:\n"});
        blk->disp(false);
        tio::tabr();
    }
    tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// StmtForIn /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtForIn::StmtForIn(ModuleLoc loc, StringRef iter, Stmt *in, StmtBlock *blk)
    : Stmt(FORIN, loc), iter(iter), in(in), blk(blk)
{}
StmtForIn::~StmtForIn() {}
StmtForIn *StmtForIn::create(ManagedList &allocator, ModuleLoc loc, StringRef iter, Stmt *in,
                             StmtBlock *blk)
{
    return allocator.alloc<StmtForIn>(loc, iter, in, blk);
}

void StmtForIn::disp(bool hasNext)
{
    tio::taba(hasNext);
    tio::print(hasNext, {"For each: ", iter, "\n"});
    tio::taba(blk);
    tio::print(blk, {"In-Expr:\n"});
    in->disp(false);
    tio::tabr();
    if(blk) {
        tio::taba(false);
        tio::print(false, {"Block:\n"});
        blk->disp(false);
        tio::tabr();
    }
    tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtRet //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtRetYield::StmtRetYield(ModuleLoc loc, Stmt *val, bool yield)
    : Stmt(RET, loc), val(val), yield(yield)
{}
StmtRetYield::~StmtRetYield() {}
StmtRetYield *StmtRetYield::create(ManagedList &allocator, ModuleLoc loc, Stmt *val, bool yield)
{
    return allocator.alloc<StmtRetYield>(loc, val, yield);
}

void StmtRetYield::disp(bool hasNext)
{
    tio::taba(hasNext);
    tio::print(hasNext, {isYield() ? "Yield\n" : "Return\n"});
    if(val) {
        tio::taba(false);
        tio::print(false, {"Value:\n"});
        val->disp(false);
        tio::tabr();
    }
    tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtContinue ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtContinue::StmtContinue(ModuleLoc loc) : Stmt(CONTINUE, loc) {}
StmtContinue *StmtContinue::create(ManagedList &allocator, ModuleLoc loc)
{
    return allocator.alloc<StmtContinue>(loc);
}

void StmtContinue::disp(bool hasNext)
{
    tio::taba(hasNext);
    tio::print(hasNext, {"Continue\n"});
    tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtBreak ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtBreak::StmtBreak(ModuleLoc loc) : Stmt(BREAK, loc) {}
StmtBreak *StmtBreak::create(ManagedList &allocator, ModuleLoc loc)
{
    return allocator.alloc<StmtBreak>(loc);
}

void StmtBreak::disp(bool hasNext)
{
    tio::taba(hasNext);
    tio::print(hasNext, {"Break\n"});
    tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtDefer ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtDefer::StmtDefer(ModuleLoc loc, Stmt *val) : Stmt(DEFER, loc), val(val) {}
StmtDefer::~StmtDefer() {}
StmtDefer *StmtDefer::create(ManagedList &allocator, ModuleLoc loc, Stmt *val)
{
    return allocator.alloc<StmtDefer>(loc, val);
}

void StmtDefer::disp(bool hasNext)
{
    tio::taba(hasNext);
    tio::print(hasNext, {"Defer\n"});
    if(val) {
        tio::taba(false);
        tio::print(false, {"Value:\n"});
        val->disp(false);
        tio::tabr();
    }
    tio::tabr();
}

} // namespace fer::ast