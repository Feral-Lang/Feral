#include "AST/Passes/Simplify.hpp"

#include "Error.hpp"

namespace fer::ast
{

bool DeferStack::popLoop(ModuleLoc loc)
{
	if(deferstack.back().size() != 1 || deferstack.back()[0] != nullptr) {
		err.fail(loc, "invalid pop loop from defer stack");
		return false;
	}
	deferstack.pop_back();
	return true;
}

void DeferStack::applyDefers(Vector<Stmt *> &stmts)
{
	// get defer statements' insert location
	size_t loc	   = 0;
	bool is_ret	   = false;
	bool is_break_cont = false;
	if(!stmts.empty()) {
		loc	      = stmts.size();
		is_ret	      = stmts[loc - 1]->isReturn();
		is_break_cont = stmts[loc - 1]->isBreak() || stmts[loc - 1]->isContinue();
		if(is_break_cont || is_ret) --loc;
	}
	for(auto stackit = deferstack.rbegin(); stackit != deferstack.rend(); ++stackit) {
		auto &layer = *stackit;
		// loop's layer is represented by layer[0] = nullptr
		// therefore, if we're at the loop layer and if it's a continue/break statement,
		// then stop applying defers
		if(is_break_cont && (layer.empty() || layer[0] == nullptr)) break;
		for(auto defit = layer.rbegin(); defit != layer.rend(); ++defit) {
			if(*defit != nullptr) stmts.insert(stmts.begin() + loc, *defit);
			++loc;
		}
		if(!is_ret && !is_break_cont) break;
	}
}

SimplifyPass::SimplifyPass(Allocator &allocator) : Pass(Pass::genPassID<SimplifyPass>(), allocator)
{}
SimplifyPass::~SimplifyPass() {}

bool SimplifyPass::visit(Stmt *stmt, Stmt **source)
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
	case FORIN: return visit(as<StmtForIn>(stmt), source);
	case RET: return visit(as<StmtRet>(stmt), source);
	case CONTINUE: return visit(as<StmtContinue>(stmt), source);
	case BREAK: return visit(as<StmtBreak>(stmt), source);
	case DEFER: return visit(as<StmtDefer>(stmt), source);
	}
	err.fail(stmt->getLoc(),
		 "invalid statement found for simplify pass: ", stmt->getStmtTypeCString());
	return false;
}

bool SimplifyPass::visit(StmtBlock *stmt, Stmt **source)
{
	defers.pushLayer();
	auto &stmts = stmt->getStmts();
	for(size_t i = 0; i < stmts.size(); ++i) {
		if(!visit(stmts[i], &stmts[i])) {
			err.fail(stmt->getLoc(),
				 "failed to perform simplify pass on stmt in block");
			return false;
		}
		if(!stmts[i]) {
			stmts.erase(stmts.begin() + i);
			--i;
			continue;
		}
	}
	defers.applyDefers(stmts);
	defers.popLayer();
	return true;
}
bool SimplifyPass::visit(StmtSimple *stmt, Stmt **source) { return true; }
bool SimplifyPass::visit(StmtFnArgs *stmt, Stmt **source)
{
	auto &args = stmt->getArgs();
	for(size_t i = 0; i < args.size(); ++i) {
		if(!visit(args[i], &args[i])) {
			err.fail(stmt->getLoc(), "failed to apply simplify pass on call argument");
			return false;
		}
	}
	return true;
}
bool SimplifyPass::visit(StmtExpr *stmt, Stmt **source)
{
	Stmt *&lhs = stmt->getLHS();
	Stmt *&rhs = stmt->getRHS();
	if(lhs && !visit(lhs, &lhs)) {
		err.fail(stmt->getLoc(), "failed to apply simplify pass on LHS in expression");
		return false;
	}
	if(rhs && !visit(rhs, &rhs)) {
		err.fail(stmt->getLoc(), "failed to apply simplify pass on LHS in expression");
		return false;
	}

	// constant folding
	if(!lhs->isSimple()) return true;
	if(rhs && !rhs->isSimple()) return true;
	StmtSimple *l = as<StmtSimple>(lhs);
	StmtSimple *r = rhs ? as<StmtSimple>(rhs) : nullptr;
	Stmt *res     = nullptr;
	if(!applyConstantFolding(res, l, r, stmt->getOper())) return false;
	if(res) *source = res;
	return true;
}
bool SimplifyPass::visit(StmtVar *stmt, Stmt **source)
{
	if(stmt->getVal() && !visit(stmt->getVal(), asStmt(&stmt->getVal()))) {
		err.fail(stmt->getLoc(),
			 "failed to apply simplify pass on var: ", stmt->getName().getDataStr());
		return false;
	}
	return true;
}
bool SimplifyPass::visit(StmtFnSig *stmt, Stmt **source)
{
	auto &args = stmt->getArgs();
	for(size_t i = 0; i < args.size(); ++i) {
		if(!visit(args[i], asStmt(&args[i]))) {
			err.fail(stmt->getLoc(),
				 "failed to apply simplify pass on function signature arg");
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
bool SimplifyPass::visit(StmtFnDef *stmt, Stmt **source)
{
	if(!visit(stmt->getSig(), asStmt(&stmt->getSig()))) {
		err.fail(stmt->getLoc(),
			 "failed to apply simplify pass on func signature in definition");
		return false;
	}
	if(!stmt->getSig()) {
		*source = nullptr;
		return true;
	}
	if(!visit(stmt->getBlk(), asStmt(&stmt->getBlk()))) {
		err.fail(stmt->getLoc(), "failed to apply simplify pass on func def block");
		return false;
	}
	return true;
}
bool SimplifyPass::visit(StmtVarDecl *stmt, Stmt **source)
{
	for(size_t i = 0; i < stmt->getDecls().size(); ++i) {
		auto &d = stmt->getDecls()[i];
		if(!visit(d, asStmt(&d))) {
			err.fail(stmt->getLoc(),
				 "failed to apply simplify pass on variable declaration");
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
bool SimplifyPass::visit(StmtCond *stmt, Stmt **source)
{
	for(auto &c : stmt->getConditionals()) {
		if(c.getCond() && !visit(c.getCond(), &c.getCond())) {
			err.fail(stmt->getLoc(),
				 "failed to apply simplify pass on conditional condition");
			return false;
		}
		if(c.getBlk() && !visit(c.getBlk(), asStmt(&c.getBlk()))) {
			err.fail(stmt->getLoc(),
				 "failed to apply simplify pass on conditional block");
			return false;
		}
	}
	return true;
}
bool SimplifyPass::visit(StmtFor *stmt, Stmt **source)
{
	if(stmt->getInit() && !visit(stmt->getInit(), &stmt->getInit())) {
		err.fail(stmt->getLoc(), "failed to apply simplify pass on for-loop init");
		return false;
	}
	if(stmt->getCond() && !visit(stmt->getCond(), &stmt->getCond())) {
		err.fail(stmt->getLoc(), "failed to apply simplify pass on for-loop cond");
		return false;
	}
	if(stmt->getIncr() && !visit(stmt->getIncr(), &stmt->getIncr())) {
		err.fail(stmt->getLoc(), "failed to apply simplify pass on for-loop incr");
		return false;
	}
	defers.pushLoop();
	if(!visit(stmt->getBlk(), asStmt(&stmt->getBlk()))) {
		err.fail(stmt->getLoc(), "failed to apply simplify pass on func def block");
		return false;
	}
	if(!defers.popLoop(stmt->getLoc())) return false;
	return true;
}
bool SimplifyPass::visit(StmtForIn *stmt, Stmt **source)
{
	if(!visit(stmt->getIn(), &stmt->getIn())) {
		err.fail(stmt->getLoc(), "failed to apply simplify pass on forin loop expr");
		return false;
	}
	defers.pushLoop();
	if(!visit(stmt->getBlk(), asStmt(&stmt->getBlk()))) {
		err.fail(stmt->getLoc(), "failed to apply simplify pass on func def block");
		return false;
	}
	if(!defers.popLoop(stmt->getLoc())) return false;
	return true;
}
bool SimplifyPass::visit(StmtRet *stmt, Stmt **source)
{
	if(stmt->getRetVal() && !visit(stmt->getRetVal(), &stmt->getRetVal())) {
		err.fail(stmt->getLoc(), "failed to apply simplify pass on return value");
		return false;
	}
	return true;
}
bool SimplifyPass::visit(StmtContinue *stmt, Stmt **source) { return true; }
bool SimplifyPass::visit(StmtBreak *stmt, Stmt **source) { return true; }
bool SimplifyPass::visit(StmtDefer *stmt, Stmt **source)
{
	defers.addStmt(stmt->getDeferVal());
	*source = nullptr;
	return true;
}

} // namespace fer::ast