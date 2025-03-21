#include "AST/Stmts.hpp"

#include "TreeIO.hpp"

namespace fer::ast
{

///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// Stmt //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt::Stmt(Stmts stmt_type, ModuleLoc loc) : loc(loc), stype(stmt_type) {}
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

StmtBlock::StmtBlock(ModuleLoc loc, const Vector<Stmt *> &stmts, bool is_top)
	: Stmt(BLOCK, loc), stmts(stmts), is_top(is_top), should_unload(true)
{}
StmtBlock::~StmtBlock() {}
StmtBlock *StmtBlock::create(Allocator &allocator, ModuleLoc loc, const Vector<Stmt *> &stmts,
			     bool is_top)
{
	return allocator.alloc<StmtBlock>(loc, stmts, is_top);
}

void StmtBlock::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Block [top = ", is_top ? "yes" : "no",
			      "][should unload = ", should_unload ? "yes" : "no", "]\n"});
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

StmtSimple::StmtSimple(ModuleLoc loc, const lex::Lexeme &val) : Stmt(SIMPLE, loc), val(val) {}

StmtSimple::~StmtSimple() {}
StmtSimple *StmtSimple::create(Allocator &allocator, ModuleLoc loc, const lex::Lexeme &val)
{
	return allocator.alloc<StmtSimple>(loc, val);
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

StmtFnArgs::StmtFnArgs(ModuleLoc loc, Vector<Stmt *> &&args, Vector<bool> &&unpack_vector)
	: Stmt(FNARGS, loc), args(args), unpack_vector(unpack_vector)
{}
StmtFnArgs::~StmtFnArgs() {}
StmtFnArgs *StmtFnArgs::create(Allocator &allocator, ModuleLoc loc, Vector<Stmt *> &&args,
			       Vector<bool> &&unpack_vector)
{
	return allocator.alloc<StmtFnArgs>(loc, std::move(args), std::move(unpack_vector));
}

void StmtFnArgs::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Function Args: ", args.empty() ? "(empty)" : "", "\n"});
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

StmtExpr::StmtExpr(ModuleLoc loc, Stmt *lhs, const lex::Lexeme &oper, Stmt *rhs)
	: Stmt(EXPR, loc), lhs(lhs), oper(oper), rhs(rhs), or_blk(nullptr), or_blk_var(loc)
{}
StmtExpr::~StmtExpr() {}
StmtExpr *StmtExpr::create(Allocator &allocator, ModuleLoc loc, Stmt *lhs, const lex::Lexeme &oper,
			   Stmt *rhs)
{
	return allocator.alloc<StmtExpr>(loc, lhs, oper, rhs);
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

StmtVar::StmtVar(ModuleLoc loc, const lex::Lexeme &name, Stmt *in, Stmt *val, bool is_arg)
	: Stmt(VAR, loc), name(name), in(in), val(val), is_arg(is_arg)
{}
StmtVar::~StmtVar() {}
StmtVar *StmtVar::create(Allocator &allocator, ModuleLoc loc, const lex::Lexeme &name, Stmt *in,
			 Stmt *val, bool is_arg)
{
	return allocator.alloc<StmtVar>(loc, name, in, val, is_arg);
}

void StmtVar::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {is_arg ? "Argument: " : "Variable: ", name.getDataStr(), "\n"});
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
		     StmtSimple *vaarg)
	: Stmt(FNSIG, loc), args(args), kwarg(kwarg), vaarg(vaarg)
{}
StmtFnSig::~StmtFnSig() {}
StmtFnSig *StmtFnSig::create(Allocator &allocator, ModuleLoc loc, const Vector<StmtVar *> &args,
			     StmtSimple *kwarg, StmtSimple *vaarg)
{
	return allocator.alloc<StmtFnSig>(loc, args, kwarg, vaarg);
}

void StmtFnSig::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Function signature\n"});
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

StmtFnDef::StmtFnDef(ModuleLoc loc, StmtFnSig *sig, StmtBlock *blk)
	: Stmt(FNDEF, loc), sig(sig), blk(blk)
{}
StmtFnDef::~StmtFnDef() {}
StmtFnDef *StmtFnDef::create(Allocator &allocator, ModuleLoc loc, StmtFnSig *sig, StmtBlock *blk)
{
	return allocator.alloc<StmtFnDef>(loc, sig, blk);
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

StmtVarDecl::StmtVarDecl(ModuleLoc loc, const Vector<StmtVar *> &decls)
	: Stmt(VARDECL, loc), decls(decls)
{}
StmtVarDecl::~StmtVarDecl() {}
StmtVarDecl *StmtVarDecl::create(Allocator &allocator, ModuleLoc loc,
				 const Vector<StmtVar *> &decls)
{
	return allocator.alloc<StmtVarDecl>(loc, decls);
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

StmtCond::StmtCond(ModuleLoc loc, const Vector<Conditional> &conds) : Stmt(COND, loc), conds(conds)
{}
StmtCond::~StmtCond() {}
StmtCond *StmtCond::create(Allocator &allocator, ModuleLoc loc, const Vector<Conditional> &conds)
{
	return allocator.alloc<StmtCond>(loc, conds);
}

void StmtCond::disp(bool has_next)
{
	bool is_inline = conds.size() > 0 && conds[0].getBlk() && conds[0].getBlk()->isTop();
	tio::taba(has_next);
	tio::print(has_next, {"Conditional [is_inline: ", is_inline ? "true" : "false", "]\n"});
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
StmtFor *StmtFor::create(Allocator &allocator, ModuleLoc loc, Stmt *init, Stmt *cond, Stmt *incr,
			 StmtBlock *blk)
{
	return allocator.alloc<StmtFor>(loc, init, cond, incr, blk);
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
/////////////////////////////////////////// StmtForIn /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtForIn::StmtForIn(ModuleLoc loc, const lex::Lexeme &iter, Stmt *in, StmtBlock *blk)
	: Stmt(FORIN, loc), iter(iter), in(in), blk(blk)
{}
StmtForIn::~StmtForIn() {}
StmtForIn *StmtForIn::create(Allocator &allocator, ModuleLoc loc, const lex::Lexeme &iter, Stmt *in,
			     StmtBlock *blk)
{
	return allocator.alloc<StmtForIn>(loc, iter, in, blk);
}

void StmtForIn::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"For each: ", iter.getDataStr(), "\n"});
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

StmtRet::StmtRet(ModuleLoc loc, Stmt *val) : Stmt(RET, loc), val(val) {}
StmtRet::~StmtRet() {}
StmtRet *StmtRet::create(Allocator &allocator, ModuleLoc loc, Stmt *val)
{
	return allocator.alloc<StmtRet>(loc, val);
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

StmtContinue::StmtContinue(ModuleLoc loc) : Stmt(CONTINUE, loc) {}
StmtContinue *StmtContinue::create(Allocator &allocator, ModuleLoc loc)
{
	return allocator.alloc<StmtContinue>(loc);
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

StmtBreak::StmtBreak(ModuleLoc loc) : Stmt(BREAK, loc) {}
StmtBreak *StmtBreak::create(Allocator &allocator, ModuleLoc loc)
{
	return allocator.alloc<StmtBreak>(loc);
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

StmtDefer::StmtDefer(ModuleLoc loc, Stmt *val) : Stmt(DEFER, loc), val(val) {}
StmtDefer::~StmtDefer() {}
StmtDefer *StmtDefer::create(Allocator &allocator, ModuleLoc loc, Stmt *val)
{
	return allocator.alloc<StmtDefer>(loc, val);
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

} // namespace fer::ast