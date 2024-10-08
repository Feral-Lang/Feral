#pragma once

// Converts the parse tree to IR (bytecode)

#include "AST/Passes/Base.hpp"

namespace fer::ast
{

class CodegenPass : public Pass
{
	// argument info string for function definitions
	// needs to be propagated between function signature and definition
	Vector<String> fndefarginfo;
	// argument info string for function call
	// needs to be propagated between function call args and expression
	Vector<String> fncallarginfo;
	// for logical AND and OR jumps
	Vector<size_t> jmplocs;
	Bytecode &bc;

public:
	CodegenPass(Allocator &allocator, Bytecode &bc);
	~CodegenPass() override;

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