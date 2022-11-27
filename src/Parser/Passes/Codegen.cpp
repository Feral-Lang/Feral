#include "Parser/Passes/Codegen.hpp"

#include "Error.hpp"

namespace fer
{

static StmtFnSig **toGenInBlock = nullptr;

CodegenParserPass::CodegenParserPass(Context &ctx, Bytecode &bc)
	: ParserPass(ParserPass::genPassID<CodegenParserPass>(), ctx), bc(bc)
{}
CodegenParserPass::~CodegenParserPass() {}

bool CodegenParserPass::visit(Stmt *stmt, Stmt **source)
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
	err::out(stmt, {"invalid statement found for codegen pass: ", stmt->getStmtTypeCString()});
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtBlock ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenParserPass::visit(StmtBlock *stmt, Stmt **source)
{
	if(!stmt->isTop()) bc.addInstrInt(Opcode::PUSH_LAYER, stmt->getLoc(), 1);
	if(toGenInBlock) {
		if(!visit(*toGenInBlock, asStmt(toGenInBlock))) {
			err::out(*toGenInBlock,
				 {"unable to generate bytecode for function signature"});
			return false;
		}
		toGenInBlock = nullptr;
	}
	for(auto &s : stmt->getStmts()) {
		if(!visit(s, &s)) {
			err::out(stmt, {"failed to generate bytecode for block stmt"});
			return false;
		}
		if(s->isExpr() || s->isSimple()) {
			bc.addInstrInt(Opcode::UNLOAD, s->getLoc(), 1);
		}
	}
	if(!stmt->isTop()) bc.addInstrInt(Opcode::POP_LAYER, stmt->getLoc(), 1);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtSimple /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenParserPass::visit(StmtSimple *stmt, Stmt **source)
{
	const lex::Lexeme &val = stmt->getLexValue();
	switch(val.getTokVal()) {
	case lex::IDEN: // fallthrough
	case lex::STR:
		bc.addInstrStr(Opcode::LOAD_CONST, stmt->getLoc(), val.getDataStr());
		return true;
	case lex::INT:
		bc.addInstrInt(Opcode::LOAD_CONST, stmt->getLoc(), val.getDataInt());
		return true;
	case lex::FLT:
		bc.addInstrFlt(Opcode::LOAD_CONST, stmt->getLoc(), val.getDataFlt());
		return true;
	case lex::CHAR:
		bc.addInstrChr(Opcode::LOAD_CONST, stmt->getLoc(), val.getDataInt());
		return true;
	case lex::TRUE: bc.addInstrBool(Opcode::LOAD_CONST, stmt->getLoc(), true); return true;
	case lex::FALSE: bc.addInstrBool(Opcode::LOAD_CONST, stmt->getLoc(), false); return true;
	case lex::NIL: bc.addInstrNil(Opcode::LOAD_CONST, stmt->getLoc()); return true;
	default: break;
	}
	err::out(stmt,
		 {"unable to generate bytecode - unknown simple type: ", val.getTok().cStr()});
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtFnArgs /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenParserPass::visit(StmtFnArgs *stmt, Stmt **source)
{
	// ssize_t because size_t can overflow
	for(ssize_t i = stmt->getArgs().size() - 1; i >= 0; --i) {
		Stmt *&a = stmt->getArg(i);
		if(!visit(a, &a)) {
			err::out(a, {"failed to generate code for function argument"});
			return false;
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtExpr /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenParserPass::visit(StmtExpr *stmt, Stmt **source)
{
	// index to edit from where the jump after RHS is to occur (for && and || operations)
	size_t logicaljmploc;
	lex::TokType oper = stmt->getOperTok().getVal();

	if(!visit(stmt->getLHS(), &stmt->getLHS())) {
		err::out(stmt->getLHS(), {"failed to generate code for LHS of expression"});
		return false;
	}

	if(oper == lex::LAND || oper == lex::LOR) {
		logicaljmploc = bc.size();
		if(oper == lex::LAND) {
			bc.addInstrInt(Opcode::JMP_FALSE, stmt->getLHS()->getLoc(), 0);
		} else {
			bc.addInstrInt(Opcode::JMP_TRUE, stmt->getLHS()->getLoc(), 0);
		}
		bc.addInstrInt(Opcode::UNLOAD, stmt->getLHS()->getLoc(), 1);
	}

	if(stmt->getRHS() && !visit(stmt->getRHS(), &stmt->getRHS())) {
		err::out(stmt->getRHS(), {"failed to generate code for RHS of expression"});
		return false;
	}

	if(oper == lex::LAND || oper == lex::LOR) bc.updateInstrInt(logicaljmploc, bc.size());

	switch(oper) {
	case lex::ASSN: bc.addInstrInt(Opcode::ASSN, stmt->getLoc(), 0); break;
	// Arithmetic
	case lex::ADD: bc.addInstrInt(Opcode::ADD, stmt->getLoc(), 0); break;
	case lex::SUB: bc.addInstrInt(Opcode::SUB, stmt->getLoc(), 0); break;
	case lex::MUL: bc.addInstrInt(Opcode::MUL, stmt->getLoc(), 0); break;
	case lex::DIV: bc.addInstrInt(Opcode::DIV, stmt->getLoc(), 0); break;
	case lex::MOD: bc.addInstrInt(Opcode::MOD, stmt->getLoc(), 0); break;
	case lex::ADD_ASSN: bc.addInstrInt(Opcode::ADD_ASSN, stmt->getLoc(), 0); break;
	case lex::SUB_ASSN: bc.addInstrInt(Opcode::SUB_ASSN, stmt->getLoc(), 0); break;
	case lex::MUL_ASSN: bc.addInstrInt(Opcode::MUL_ASSN, stmt->getLoc(), 0); break;
	case lex::DIV_ASSN: bc.addInstrInt(Opcode::DIV_ASSN, stmt->getLoc(), 0); break;
	case lex::MOD_ASSN: bc.addInstrInt(Opcode::MOD_ASSN, stmt->getLoc(), 0); break;
	// Post/Pre Inc/Dec
	case lex::XINC: bc.addInstrInt(Opcode::XINC, stmt->getLoc(), 0); break;
	case lex::INCX: bc.addInstrInt(Opcode::INCX, stmt->getLoc(), 0); break;
	case lex::XDEC: bc.addInstrInt(Opcode::XDEC, stmt->getLoc(), 0); break;
	case lex::DECX: bc.addInstrInt(Opcode::DECX, stmt->getLoc(), 0); break;
	// Unary
	case lex::UADD: bc.addInstrInt(Opcode::UADD, stmt->getLoc(), 0); break;
	case lex::USUB: bc.addInstrInt(Opcode::USUB, stmt->getLoc(), 0); break;
	case lex::UAND: bc.addInstrInt(Opcode::UAND, stmt->getLoc(), 0); break;
	case lex::UMUL: bc.addInstrInt(Opcode::UMUL, stmt->getLoc(), 0); break;
	// Logic (LAND and LOR are handled using jmps)
	case lex::LAND: break;
	case lex::LOR: break;
	case lex::LNOT: bc.addInstrInt(Opcode::LNOT, stmt->getLoc(), 0); break;
	// Comparison
	case lex::EQ: bc.addInstrInt(Opcode::EQ, stmt->getLoc(), 0); break;
	case lex::LT: bc.addInstrInt(Opcode::LT, stmt->getLoc(), 0); break;
	case lex::GT: bc.addInstrInt(Opcode::GT, stmt->getLoc(), 0); break;
	case lex::LE: bc.addInstrInt(Opcode::LE, stmt->getLoc(), 0); break;
	case lex::GE: bc.addInstrInt(Opcode::GE, stmt->getLoc(), 0); break;
	case lex::NE: bc.addInstrInt(Opcode::NE, stmt->getLoc(), 0); break;
	// Bitwise
	case lex::BAND: bc.addInstrInt(Opcode::BAND, stmt->getLoc(), 0); break;
	case lex::BOR: bc.addInstrInt(Opcode::BOR, stmt->getLoc(), 0); break;
	case lex::BNOT: bc.addInstrInt(Opcode::BNOT, stmt->getLoc(), 0); break;
	case lex::BXOR: bc.addInstrInt(Opcode::BXOR, stmt->getLoc(), 0); break;
	case lex::BAND_ASSN: bc.addInstrInt(Opcode::BAND_ASSN, stmt->getLoc(), 0); break;
	case lex::BOR_ASSN: bc.addInstrInt(Opcode::BOR_ASSN, stmt->getLoc(), 0); break;
	case lex::BNOT_ASSN: bc.addInstrInt(Opcode::BNOT_ASSN, stmt->getLoc(), 0); break;
	case lex::BXOR_ASSN: bc.addInstrInt(Opcode::BXOR_ASSN, stmt->getLoc(), 0); break;
	// Others
	case lex::LSHIFT: bc.addInstrInt(Opcode::LSHIFT, stmt->getLoc(), 0); break;
	case lex::RSHIFT: bc.addInstrInt(Opcode::RSHIFT, stmt->getLoc(), 0); break;
	case lex::LSHIFT_ASSN: bc.addInstrInt(Opcode::LSHIFT_ASSN, stmt->getLoc(), 0); break;
	case lex::RSHIFT_ASSN: bc.addInstrInt(Opcode::RSHIFT_ASSN, stmt->getLoc(), 0); break;
	case lex::SUBS: bc.addInstrInt(Opcode::SUBS, stmt->getLoc(), 0); break;
	case lex::DOT: bc.addInstrInt(Opcode::DOT, stmt->getLoc(), 0); break;
	case lex::FNCALL: {
		// TODO: technically an assert
		if(!stmt->getRHS()->isFnArgs()) {
			err::out(stmt->getRHS(), {"unexpected internal failure: fnargs expected "
						  "but not found during codegen"});
			return false;
		}
		size_t args = as<StmtFnArgs>(stmt->getRHS())->getArgs().size();
		bc.addInstrInt(Opcode::FNCALL, stmt->getLoc(), args);
		break;
	}
	default: {
		err::out(stmt, {"invalid operator during codegen: ",
				lex::TokStrs[stmt->getOperTok().getVal()]});
		return false;
	}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtVar //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenParserPass::visit(StmtVar *stmt, Stmt **source)
{
	Stmt *&val		= stmt->getVal();
	const lex::Lexeme &name = stmt->getName();
	if(!val) {
		err::out(stmt, {"cannot generate bytecode of"
				" a variable with no value: ",
				name.getDataStr()});
		return false;
	}
	if(!visit(val, &val)) {
		err::out(stmt,
			 {"failed to generate bytecode of variable val: ", name.getDataStr()});
		return false;
	}
	if(stmt->isConst()) bc.addInstrStr(Opcode::CREATE_CONST, stmt->getLoc(), name.getDataStr());
	else bc.addInstrStr(Opcode::CREATE_VAR, stmt->getLoc(), name.getDataStr());
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFnSig ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenParserPass::visit(StmtFnSig *stmt, Stmt **source)
{
	for(ssize_t i = stmt->getArgs().size() - 1; i >= 0; --i) {
		auto &arg = stmt->getArg(i);
		bc.addInstrInt(Opcode::LOAD_ARG, arg->getLoc(), i);
		if(arg->isConst()) {
			bc.addInstrStr(Opcode::CREATE_CONST, arg->getLoc(),
				       arg->getName().getDataStr());
		} else {
			bc.addInstrStr(Opcode::CREATE_VAR, arg->getLoc(),
				       arg->getName().getDataStr());
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFnDef ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenParserPass::visit(StmtFnDef *stmt, Stmt **source)
{
	toGenInBlock	      = &stmt->getSig();
	size_t block_till_loc = bc.size();
	bc.addInstrInt(Opcode::BLOCK_TILL, stmt->getLoc(), 0); // 0 is a placeholder
	if(!visit(stmt->getBlk(), asStmt(&stmt->getBlk()))) {
		err::out(stmt, {"failed to generate code for function definition block"});
		return false;
	}
	bc.updateInstrInt(block_till_loc, bc.size() - 1);
	// toGenInBlock is cleared in StmtBlock::visit()
	size_t arginfo = stmt->getSigArgs().size();
	if(stmt->isSigVariadic()) arginfo = addVariadicFlag(arginfo);
	bc.addInstrInt(Opcode::CREATE_FN, stmt->getLoc(), arginfo);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// StmtVarDecl /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenParserPass::visit(StmtVarDecl *stmt, Stmt **source)
{
	for(auto &d : stmt->getDecls()) {
		if(!visit(d, asStmt(&d))) return false;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtCond /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenParserPass::visit(StmtCond *stmt, Stmt **source)
{
	Vector<size_t> jmpendplaceholders;
	for(auto &c : stmt->getConditionals()) {
		if(c.getCond()) {
			if(!visit(c.getCond(), &c.getCond())) {
				err::out(c.getCond(), {"failed to generate code for condition"});
				return false;
			}
			jmpendplaceholders.push_back(bc.size());
			bc.addInstrInt(Opcode::JMP_FALSE_POP, c.getCond()->getLoc(), 0);
		}
		if(!visit(c.getBlk(), asStmt(&c.getBlk()))) {
			err::out(stmt, {"failed to generate code for conditional block"});
			return false;
		}
	}
	for(auto &placeholder : jmpendplaceholders) {
		bc.updateInstrInt(placeholder, bc.size());
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFor //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenParserPass::visit(StmtFor *stmt, Stmt **source)
{
	Stmt *&init	= stmt->getInit();
	Stmt *&cond	= stmt->getCond();
	Stmt *&incr	= stmt->getIncr();
	StmtBlock *&blk = stmt->getBlk();

	size_t condloc	  = 0;
	size_t condjmploc = 0;

	bc.addInstrInt(Opcode::PUSH_LAYER, stmt->getLoc(), 1);
	if(init) {
		if(!visit(init, &init)) {
			err::out(init, {"failed to generate code for loop init"});
			return false;
		}
		if(init->isExpr() || init->isSimple()) {
			bc.addInstrInt(Opcode::UNLOAD, init->getLoc(), 1);
		}
	}
	condloc = bc.size();
	if(cond) {
		if(!visit(cond, &cond)) {
			err::out(cond, {"failed to generate code for loop init"});
			return false;
		}
		condjmploc = bc.size();
		bc.addInstrInt(Opcode::JMP_FALSE_POP, cond->getLoc(), 0); // placeholder
	}

	if(blk && !visit(blk, asStmt(&blk))) {
		err::out(blk, {"failed to generate code for loop block"});
		return false;
	}

	if(incr) {
		if(!visit(incr, &incr)) {
			err::out(incr, {"failed to generate code for loop init"});
			return false;
		}
		if(incr->isExpr() || incr->isSimple()) {
			bc.addInstrInt(Opcode::UNLOAD, incr->getLoc(), 1);
		}
	}
	bc.addInstrInt(Opcode::JMP, stmt->getLoc(), condloc); // jmp back to condition
	if(cond) bc.updateInstrInt(condjmploc, bc.size());
	bc.addInstrInt(Opcode::POP_LAYER, stmt->getLoc(), 1);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtRet //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenParserPass::visit(StmtRet *stmt, Stmt **source)
{
	if(stmt->getRetVal() && !visit(stmt->getRetVal(), &stmt->getRetVal())) {
		err::out(stmt->getRetVal(), {"failed to generate code for return value"});
		return false;
	}
	bc.addInstrBool(Opcode::RETURN, stmt->getLoc(), stmt->getRetVal());
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtContinue ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenParserPass::visit(StmtContinue *stmt, Stmt **source)
{
	bc.addInstrInt(Opcode::CONTINUE, stmt->getLoc(), 0);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtBreak ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenParserPass::visit(StmtBreak *stmt, Stmt **source)
{
	bc.addInstrInt(Opcode::BREAK, stmt->getLoc(), 0);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtDefer ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenParserPass::visit(StmtDefer *stmt, Stmt **source)
{
	err::out(stmt, {"defer should have been dealt with in SimplifyParserPass"});
	return false;
}

} // namespace fer