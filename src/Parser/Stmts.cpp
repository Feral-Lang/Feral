#include "Parser/Stmts.hpp"

#include "TreeIO.hpp"

namespace fer
{

///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// Stmt //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt::Stmt(Stmts stmt_type, const ModuleLoc *loc) : loc(loc), stype(stmt_type) {}
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

StmtBlock::StmtBlock(const ModuleLoc *loc, const Vector<Stmt *> &stmts, bool is_top)
	: Stmt(BLOCK, loc), stmts(stmts), is_top(is_top)
{}
StmtBlock::~StmtBlock() {}
StmtBlock *StmtBlock::create(Context &c, const ModuleLoc *loc, const Vector<Stmt *> &stmts,
			     bool is_top)
{
	return c.allocStmt<StmtBlock>(loc, stmts, is_top);
}

void StmtBlock::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Block [top = ", is_top ? "yes" : "no", "]\n"});
	for(size_t i = 0; i < stmts.size(); ++i) {
		if(!stmts[i]) {
			tio::taba(has_next);
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

StmtSimple::StmtSimple(const ModuleLoc *loc, const lex::Lexeme &val) : Stmt(SIMPLE, loc), val(val)
{}

StmtSimple::~StmtSimple() {}
StmtSimple *StmtSimple::create(Context &c, const ModuleLoc *loc, const lex::Lexeme &val)
{
	return c.allocStmt<StmtSimple>(loc, val);
}

void StmtSimple::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Simple: ", val.str(0), "\n"});
	tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtFnArgs /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFnArgs::StmtFnArgs(const ModuleLoc *loc, const Vector<Stmt *> &args)
	: Stmt(FNARGS, loc), args(args)
{}
StmtFnArgs::~StmtFnArgs() {}
StmtFnArgs *StmtFnArgs::create(Context &c, const ModuleLoc *loc, const Vector<Stmt *> &args)
{
	return c.allocStmt<StmtFnArgs>(loc, args);
}

void StmtFnArgs::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Function Call Info: ", args.empty() ? "(empty)" : "", "\n"});
	if(!args.empty()) {
		tio::taba(false);
		tio::print(false, {"Args:\n"});
		for(size_t i = 0; i < args.size(); ++i) {
			args[i]->disp(i != args.size() - 1);
		}
		tio::tabr();
	}
	tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtExpr /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtExpr::StmtExpr(const ModuleLoc *loc, Stmt *lhs, const lex::Lexeme &oper, Stmt *rhs)
	: Stmt(EXPR, loc), lhs(lhs), oper(oper), rhs(rhs), or_blk(nullptr), or_blk_var(loc)
{}
StmtExpr::~StmtExpr() {}
StmtExpr *StmtExpr::create(Context &c, const ModuleLoc *loc, Stmt *lhs, const lex::Lexeme &oper,
			   Stmt *rhs)
{
	return c.allocStmt<StmtExpr>(loc, lhs, oper, rhs);
}

void StmtExpr::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Expression\n"});
	if(lhs) {
		tio::taba(oper.getTok().isValid() || rhs || or_blk);
		tio::print(oper.getTok().isValid() || rhs || or_blk, {"LHS:\n"});
		lhs->disp(false);
		tio::tabr();
	}
	if(oper.getTok().isValid()) {
		tio::taba(rhs || or_blk);
		tio::print(rhs || or_blk, {"Oper: ", oper.getTok().cStr(), "\n"});
		tio::tabr();
	}
	if(rhs) {
		tio::taba(or_blk);
		tio::print(or_blk, {"RHS:\n"});
		rhs->disp(false);
		tio::tabr();
	}
	if(or_blk) {
		tio::taba(false);
		StringRef orblkvardata =
		or_blk_var.getTok().isData() ? or_blk_var.getDataStr() : "<none>";
		tio::print(false, {"Or: ", orblkvardata, "\n"});
		or_blk->disp(false);
		tio::tabr();
	}
	tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtVar //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtVar::StmtVar(const ModuleLoc *loc, const lex::Lexeme &name, Stmt *val)
	: Stmt(VAR, loc), name(name), val(val)
{}
StmtVar::~StmtVar() {}
StmtVar *StmtVar::create(Context &c, const ModuleLoc *loc, const lex::Lexeme &name, Stmt *val)
{
	return c.allocStmt<StmtVar>(loc, name, val);
}

void StmtVar::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Variable: ", name.getDataStr(), "\n"});
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

StmtFnSig::StmtFnSig(const ModuleLoc *loc, Vector<StmtSimple *> &args, bool is_variadic)
	: Stmt(FNSIG, loc), args(args), is_variadic(is_variadic)
{}
StmtFnSig::~StmtFnSig() {}
StmtFnSig *StmtFnSig::create(Context &c, const ModuleLoc *loc, Vector<StmtSimple *> &args,
			     bool is_variadic)
{
	return c.allocStmt<StmtFnSig>(loc, args, is_variadic);
}

void StmtFnSig::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next,
		   {"Function signature [variadic = ", is_variadic ? "yes" : "no", "]\n"});
	if(args.size() > 0) {
		tio::taba(false);
		tio::print(false, {"Parameters:\n"});
		for(size_t i = 0; i < args.size(); ++i) {
			args[i]->disp(i != args.size() - 1);
		}
		tio::tabr();
	}
	tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFnDef ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFnDef::StmtFnDef(const ModuleLoc *loc, StmtFnSig *sig, StmtBlock *blk)
	: Stmt(FNDEF, loc), sig(sig), blk(blk)
{}
StmtFnDef::~StmtFnDef() {}
StmtFnDef *StmtFnDef::create(Context &c, const ModuleLoc *loc, StmtFnSig *sig, StmtBlock *blk)
{
	return c.allocStmt<StmtFnDef>(loc, sig, blk);
}

void StmtFnDef::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Function definition\n"});
	tio::taba(true);
	tio::print(true, {"Function Signature:\n"});
	sig->disp(false);
	tio::tabr();

	tio::taba(false);
	tio::print(false, {"Function Block:\n"});
	blk->disp(false);
	tio::tabr(2);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// StmtVarDecl /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtVarDecl::StmtVarDecl(const ModuleLoc *loc, const Vector<StmtVar *> &decls)
	: Stmt(VARDECL, loc), decls(decls)
{}
StmtVarDecl::~StmtVarDecl() {}
StmtVarDecl *StmtVarDecl::create(Context &c, const ModuleLoc *loc, const Vector<StmtVar *> &decls)
{
	return c.allocStmt<StmtVarDecl>(loc, decls);
}

void StmtVarDecl::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Variable declarations\n"});
	for(size_t i = 0; i < decls.size(); ++i) {
		decls[i]->disp(i != decls.size() - 1);
	}
	tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtCond /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Conditional::Conditional(Stmt *cond, StmtBlock *blk) : cond(cond), blk(blk) {}
Conditional::~Conditional() {}

StmtCond::StmtCond(const ModuleLoc *loc, const Vector<Conditional> &conds)
	: Stmt(COND, loc), conds(conds)
{}
StmtCond::~StmtCond() {}
StmtCond *StmtCond::create(Context &c, const ModuleLoc *loc, const Vector<Conditional> &conds)
{
	return c.allocStmt<StmtCond>(loc, conds);
}

void StmtCond::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Conditional\n"});
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

StmtFor::StmtFor(const ModuleLoc *loc, Stmt *init, Stmt *cond, Stmt *incr, StmtBlock *blk)
	: Stmt(FOR, loc), init(init), cond(cond), incr(incr), blk(blk)
{}
StmtFor::~StmtFor() {}
StmtFor *StmtFor::create(Context &c, const ModuleLoc *loc, Stmt *init, Stmt *cond, Stmt *incr,
			 StmtBlock *blk)
{
	return c.allocStmt<StmtFor>(loc, init, cond, incr, blk);
}

void StmtFor::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"For/While\n"});
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
//////////////////////////////////////////// StmtRet //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtRet::StmtRet(const ModuleLoc *loc, Stmt *val) : Stmt(RET, loc), val(val) {}
StmtRet::~StmtRet() {}
StmtRet *StmtRet::create(Context &c, const ModuleLoc *loc, Stmt *val)
{
	return c.allocStmt<StmtRet>(loc, val);
}

void StmtRet::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Return\n"});
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

StmtContinue::StmtContinue(const ModuleLoc *loc) : Stmt(CONTINUE, loc) {}
StmtContinue *StmtContinue::create(Context &c, const ModuleLoc *loc)
{
	return c.allocStmt<StmtContinue>(loc);
}

void StmtContinue::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Continue\n"});
	tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtBreak ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtBreak::StmtBreak(const ModuleLoc *loc) : Stmt(BREAK, loc) {}
StmtBreak *StmtBreak::create(Context &c, const ModuleLoc *loc)
{
	return c.allocStmt<StmtBreak>(loc);
}

void StmtBreak::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Break\n"});
	tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtDefer ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtDefer::StmtDefer(const ModuleLoc *loc, Stmt *val) : Stmt(DEFER, loc), val(val) {}
StmtDefer::~StmtDefer() {}
StmtDefer *StmtDefer::create(Context &c, const ModuleLoc *loc, Stmt *val)
{
	return c.allocStmt<StmtDefer>(loc, val);
}

void StmtDefer::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Defer\n"});
	if(val) {
		tio::taba(false);
		tio::print(false, {"Value:\n"});
		val->disp(false);
		tio::tabr();
	}
	tio::tabr();
}

} // namespace fer