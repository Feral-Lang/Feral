#pragma once

// Implements the basic parse tree simplification - including constant folding
// After this pass, StmtDefer will not be found in the code

#include "AST/Passes/Base.hpp"

namespace fer::ast
{

class DeferStack
{
    Vector<Vector<Stmt *>> deferstack;

public:
    inline void pushLayer() { deferstack.push_back({}); }
    inline void popLayer() { deferstack.pop_back(); }
    inline void pushLoop() { deferstack.push_back({nullptr}); }
    bool popLoop(ModuleLoc loc);
    inline void addStmt(Stmt *stmt) { deferstack.back().push_back(stmt); }
    void applyDefers(Vector<Stmt *> &stmts);
};

class SimplifyPass : public Pass
{
    bool applyConstantFolding(Stmt *&resultStmt, StmtSimple *l, StmtSimple *r, lex::Lexeme *oper);
    DeferStack defers;

public:
    SimplifyPass(ManagedList &allocator);
    ~SimplifyPass() override;

    bool visit(Stmt *stmt, Stmt **source) override;

    bool visit(StmtBlock *stmt, Stmt **source) override;
    bool visit(StmtSimple *stmt, Stmt **source) override;
    bool visit(StmtFnArgs *stmt, Stmt **source) override;
    bool visit(StmtExpr *stmt, Stmt **source) override;
    bool visit(StmtVar *stmt, Stmt **source) override;
    bool visit(StmtFnSig *stmt, Stmt **source) override;
    bool visit(StmtFnDef *stmt, Stmt **source) override;
    bool visit(StmtVarDecl *stmt, Stmt **source) override;
    bool visit(StmtCond *stmt, Stmt **source) override;
    bool visit(StmtFor *stmt, Stmt **source) override;
    bool visit(StmtForIn *stmt, Stmt **source) override;
    bool visit(StmtRet *stmt, Stmt **source) override;
    bool visit(StmtContinue *stmt, Stmt **source) override;
    bool visit(StmtBreak *stmt, Stmt **source) override;
    bool visit(StmtDefer *stmt, Stmt **source) override;
};

} // namespace fer::ast