#include "Parser/Passes/Simplify.hpp"

#include "Error.hpp"
#include "Utils.hpp"

namespace fer
{

SimplifyParserPass::SimplifyParserPass(Context &ctx)
	: ParserPass(ParserPass::genPassID<SimplifyParserPass>(), ctx)
{}
SimplifyParserPass::~SimplifyParserPass() {}

bool SimplifyParserPass::visit(Stmt *stmt, Stmt **source)
{
	switch(stmt->getStmtType()) {
	case BLOCK: return visit(as<StmtBlock>(stmt), source);
	case SIMPLE: return visit(as<StmtSimple>(stmt), source);
	case EXPR: return visit(as<StmtExpr>(stmt), source);
	case FNARGS: return visit(as<StmtFnArgs>(stmt), source);
	case VAR: return visit(as<StmtVar>(stmt), source);
	case FNSIG: return visit(as<StmtFnSig>(stmt), source);
	case FNDEF: return visit(as<StmtFnDef>(stmt), source);
	case VARDECL: return visit(as<StmtVarDecl>(stmt), source);
	case COND: return visit(as<StmtCond>(stmt), source);
	case FOR: return visit(as<StmtFor>(stmt), source);
	case RET: return visit(as<StmtRet>(stmt), source);
	case CONTINUE: return visit(as<StmtContinue>(stmt), source);
	case BREAK: return visit(as<StmtBreak>(stmt), source);
	case DEFER: return visit(as<StmtDefer>(stmt), source);
	}
	err::out(stmt, {"invalid statement found for simplify pass: ", stmt->getStmtTypeCString()});
	return false;
}

bool SimplifyParserPass::visit(StmtBlock *stmt, Stmt **source)
{
	auto &stmts = stmt->getStmts();
	for(size_t i = 0; i < stmts.size(); ++i) {
		if(!visit(stmts[i], &stmts[i])) {
			err::out(stmt, {"failed to perform simplify pass on stmt in block"});
			return false;
		}
		if(!stmts[i]) {
			stmts.erase(stmts.begin() + i);
			--i;
			continue;
		}
		if(stmts[i]->getStmtType() == BLOCK && stmt->isTop()) {
			StmtBlock *inner = as<StmtBlock>(stmts[i]);
			stmts.erase(stmts.begin() + i);
			stmts.insert(stmts.begin() + i, inner->getStmts().begin(),
				     inner->getStmts().end());
			i += inner->getStmts().size();
			--i;
		}
	}
	return true;
}
bool SimplifyParserPass::visit(StmtSimple *stmt, Stmt **source) { return true; }
bool SimplifyParserPass::visit(StmtFnArgs *stmt, Stmt **source)
{
	auto &args = stmt->getArgs();
	for(size_t i = 0; i < args.size(); ++i) {
		if(!visit(args[i], &args[i])) {
			err::out(stmt, {"failed to apply simplify pass on call argument"});
			return false;
		}
	}
	return true;
}
bool SimplifyParserPass::visit(StmtExpr *stmt, Stmt **source)
{
	Stmt *&lhs	  = stmt->getLHS();
	Stmt *&rhs	  = stmt->getRHS();
	lex::TokType oper = stmt->getOper().getTokVal();
	if(lhs && !visit(lhs, &lhs)) {
		err::out(stmt, {"failed to apply simplify pass on LHS in expression"});
		return false;
	}
	if(rhs && !visit(rhs, &rhs)) {
		err::out(stmt, {"failed to apply simplify pass on LHS in expression"});
		return false;
	}

	// constant folding
	if(!lhs->isSimple()) return true;
	if(rhs && !rhs->isSimple()) return true;
	StmtSimple *l = as<StmtSimple>(lhs);
	StmtSimple *r = rhs ? as<StmtSimple>(rhs) : nullptr;
	if(Stmt *res = applyConstantFolding(l, r, oper)) *source = res;
	return true;
}
bool SimplifyParserPass::visit(StmtVar *stmt, Stmt **source)
{
	if(!visit(stmt->getVal(), asStmt(&stmt->getVal()))) {
		err::out(stmt,
			 {"failed to apply simplify pass on var: ", stmt->getName().getDataStr()});
		return false;
	}
	return true;
}
bool SimplifyParserPass::visit(StmtFnSig *stmt, Stmt **source)
{
	auto &args = stmt->getArgs();
	for(size_t i = 0; i < args.size(); ++i) {
		if(!visit(args[i], asStmt(&args[i]))) {
			err::out(stmt, {"failed to apply simplify pass on function signature arg"});
			return false;
		}
		if(!args[i]) {
			args.erase(args.begin() + i);
			--i;
			continue;
		}
	}
	return true;
}
bool SimplifyParserPass::visit(StmtFnDef *stmt, Stmt **source)
{
	if(!visit(stmt->getSig(), asStmt(&stmt->getSig()))) {
		err::out(stmt, {"failed to apply simplify pass on func signature in definition"});
		return false;
	}
	if(!stmt->getSig()) {
		*source = nullptr;
		return true;
	}
	if(!visit(stmt->getBlk(), asStmt(&stmt->getBlk()))) {
		err::out(stmt, {"failed to apply simplify pass on func def block"});
		return false;
	}
	return true;
}
bool SimplifyParserPass::visit(StmtVarDecl *stmt, Stmt **source)
{
	for(size_t i = 0; i < stmt->getDecls().size(); ++i) {
		auto &d = stmt->getDecls()[i];
		if(!visit(d, asStmt(&d))) {
			err::out(stmt, {"failed to apply simplify pass on variable declaration"});
			return false;
		}
		if(!d) {
			stmt->getDecls().erase(stmt->getDecls().begin() + i);
			--i;
		}
	}
	if(stmt->getDecls().empty()) {
		*source = nullptr;
		return true;
	}
	return true;
}
bool SimplifyParserPass::visit(StmtCond *stmt, Stmt **source)
{
	for(auto &c : stmt->getConditionals()) {
		if(c.getCond() && !visit(c.getCond(), &c.getCond())) {
			err::out(stmt, {"failed to apply simplify pass on conditional condition"});
			return false;
		}
		if(c.getBlk() && !visit(c.getBlk(), asStmt(&c.getBlk()))) {
			err::out(stmt, {"failed to apply simplify pass on conditional block"});
			return false;
		}
	}
	return true;
}
bool SimplifyParserPass::visit(StmtFor *stmt, Stmt **source)
{
	if(stmt->getInit() && !visit(stmt->getInit(), &stmt->getInit())) {
		err::out(stmt, {"failed to apply simplify pass on for-loop init"});
		return false;
	}
	if(stmt->getCond() && !visit(stmt->getCond(), &stmt->getCond())) {
		err::out(stmt, {"failed to apply simplify pass on for-loop cond"});
		return false;
	}
	if(stmt->getIncr() && !visit(stmt->getIncr(), &stmt->getIncr())) {
		err::out(stmt, {"failed to apply simplify pass on for-loop incr"});
		return false;
	}
	if(!visit(stmt->getBlk(), asStmt(&stmt->getBlk()))) {
		err::out(stmt, {"failed to apply simplify pass on func def block"});
		return false;
	}
	return true;
}
bool SimplifyParserPass::visit(StmtRet *stmt, Stmt **source)
{
	if(stmt->getRetVal() && !visit(stmt->getRetVal(), &stmt->getRetVal())) {
		err::out(stmt, {"failed to apply simplify pass on return value"});
		return false;
	}
	return true;
}
bool SimplifyParserPass::visit(StmtContinue *stmt, Stmt **source) { return true; }
bool SimplifyParserPass::visit(StmtBreak *stmt, Stmt **source) { return true; }
bool SimplifyParserPass::visit(StmtDefer *stmt, Stmt **source) { return true; }

} // namespace fer