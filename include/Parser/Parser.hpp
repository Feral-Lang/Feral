#pragma once

#include "ParseHelper.hpp"
#include "Stmts.hpp"

namespace fer
{

class Parser
{
	Context &ctx;

public:
	Parser(Context &ctx);

	// on successful parse, returns true, and tree is allocated
	// if with_brace is true, it will attempt to find the beginning and ending brace for each
	// block
	bool parseBlock(ParseHelper &p, StmtBlock *&tree, bool with_brace = true);

	bool parseSimple(ParseHelper &p, Stmt *&data);

	bool parsePrefixedSuffixedLiteral(ParseHelper &p, Stmt *&expr);
	bool parseExpr(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr17(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr16(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr15(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr14(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr13(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr12(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr11(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr10(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr09(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr08(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr07(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr06(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr05(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr04(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr03(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr02(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr01(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);

	// is_fn_arg is for function signature args (to make value optional)
	bool parseVar(ParseHelper &p, StmtVar *&var, bool is_fn_arg);

	bool parseFnSig(ParseHelper &p, Stmt *&fsig);
	bool parseFnDef(ParseHelper &p, Stmt *&fndef);

	bool parseVardecl(ParseHelper &p, Stmt *&vd); // combines VAR_DECL_BASE and VAR_DECL

	bool parseConds(ParseHelper &p, Stmt *&conds);
	// this is just a transformation that generates a for loop
	bool parseForIn(ParseHelper &p, Stmt *&fin);
	bool parseFor(ParseHelper &p, Stmt *&f);
	bool parseWhile(ParseHelper &p, Stmt *&w);
	bool parseRet(ParseHelper &p, Stmt *&ret);
	bool parseContinue(ParseHelper &p, Stmt *&cont);
	bool parseBreak(ParseHelper &p, Stmt *&brk);
	bool parseDefer(ParseHelper &p, Stmt *&defer);
};

} // namespace fer