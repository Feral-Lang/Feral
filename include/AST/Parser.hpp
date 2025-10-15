#pragma once

#include "ParseHelper.hpp"
#include "Stmts.hpp"

namespace fer::ast
{

class Parser
{
	ManagedAllocator &allocator;
	ParseHelper p;

public:
	Parser(ManagedAllocator &allocator, Vector<lex::Lexeme> &toks);

	// on successful parse, returns true, and tree is allocated
	// if with_brace is true, it will attempt to find the beginning and ending brace for each
	// block
	bool parseBlock(StmtBlock *&tree, bool with_brace = true);

	bool parseSimple(Stmt *&data);

	bool parsePrefixedSuffixedLiteral(Stmt *&expr);
	bool parseExpr(Stmt *&expr);
	bool parseExpr18(Stmt *&expr);
	bool parseExpr17(Stmt *&expr);
	bool parseExpr16(Stmt *&expr);
	bool parseExpr15(Stmt *&expr);
	bool parseExpr14(Stmt *&expr);
	bool parseExpr13(Stmt *&expr);
	bool parseExpr12(Stmt *&expr);
	bool parseExpr11(Stmt *&expr);
	bool parseExpr10(Stmt *&expr);
	bool parseExpr09(Stmt *&expr);
	bool parseExpr08(Stmt *&expr);
	bool parseExpr07(Stmt *&expr);
	bool parseExpr06(Stmt *&expr);
	bool parseExpr05(Stmt *&expr);
	bool parseExpr04(Stmt *&expr);
	bool parseExpr03(Stmt *&expr);
	bool parseExpr02(Stmt *&expr);
	bool parseExpr01(Stmt *&expr);

	// is_fn_arg is for function signature args (to make value optional)
	bool parseVar(StmtVar *&var, bool is_fn_arg);

	bool parseFnSig(Stmt *&fsig);
	bool parseFnDef(Stmt *&fndef);

	bool parseVardecl(Stmt *&vd); // combines VAR_DECL_BASE and VAR_DECL

	bool parseConds(Stmt *&conds);
	// this is just a transformation that generates a for loop
	bool parseForIn(Stmt *&fin);
	bool parseFor(Stmt *&f);
	bool parseWhile(Stmt *&w);
	bool parseRet(Stmt *&ret);
	bool parseContinue(Stmt *&cont);
	bool parseBreak(Stmt *&brk);
	bool parseDefer(Stmt *&defer);
};

// Can modify toks since some stuff requires it (like determining pre/post operators)
bool parse(ManagedAllocator &allocator, Vector<lex::Lexeme> &toks, Stmt *&s, bool exprOnly);
void dumpTree(OStream &os, Stmt *tree);

} // namespace fer::ast