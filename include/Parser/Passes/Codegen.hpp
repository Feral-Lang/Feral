#pragma once

// Converts the parse tree to IR (bytecode)

#include "Parser/Passes/Base.hpp"

namespace fer
{

class CodegenParserPass : public ParserPass
{
	// argument info string for function definitions
	// needs to be propagated between function signature and definition
	Vector<StringRef> fndefarginfo;
	// argument info string for function call
	// needs to be propagated between function call args and expression
	Vector<StringRef> fncallarginfo;
	Bytecode &bc;

public:
	CodegenParserPass(Context &ctx, Bytecode &bc);
	~CodegenParserPass() override;

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
	bool visit(StmtRet *stmt, Stmt **source) override;
	bool visit(StmtContinue *stmt, Stmt **source) override;
	bool visit(StmtBreak *stmt, Stmt **source) override;
	bool visit(StmtDefer *stmt, Stmt **source) override;
};

} // namespace fer