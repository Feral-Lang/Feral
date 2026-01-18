#include "AST/Passes/Codegen.hpp"

#include "Error.hpp"

namespace fer::ast
{

CodegenPass::CodegenPass(ManagedAllocator &allocator, Bytecode &bc)
    : Pass(Pass::genPassID<CodegenPass>(), allocator), bc(bc)
{}
CodegenPass::~CodegenPass() {}

bool CodegenPass::visit(Stmt *stmt, Stmt **source)
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
             "invalid statement found for codegen pass: ", stmt->getStmtTypeCString());
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtBlock ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenPass::visit(StmtBlock *stmt, Stmt **source)
{
    if(!stmt->isTop()) bc.addInstrInt(Opcode::PUSH_BLOCK, stmt->getLoc(), 1);
    for(auto &s : stmt->getStmts()) {
        if(!visit(s, &s)) {
            err.fail(stmt->getLoc(), "failed to generate bytecode for block stmt");
            return false;
        }
        if(stmt->shouldUnload() && (s->isExpr() || s->isSimple())) {
            bc.addInstrInt(Opcode::UNLOAD, s->getLoc(), 1);
        }
    }
    if(!stmt->isTop()) bc.addInstrInt(Opcode::POP_BLOCK, stmt->getLoc(), 1);
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtSimple /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenPass::visit(StmtSimple *stmt, Stmt **source)
{
    const lex::Lexeme &val = stmt->getLexValue();
    switch(val.getTokVal()) {
    case lex::IDEN:
        bc.addInstrIden(Opcode::LOAD_DATA, stmt->getLoc(), val.getDataStr());
        return true;
    case lex::STR:
        bc.addInstrStr(Opcode::LOAD_DATA, stmt->getLoc(), utils::fromRawString(val.getDataStr()));
        return true;
    case lex::INT: bc.addInstrInt(Opcode::LOAD_DATA, stmt->getLoc(), val.getDataInt()); return true;
    case lex::FLT: bc.addInstrFlt(Opcode::LOAD_DATA, stmt->getLoc(), val.getDataFlt()); return true;
    case lex::FTRUE: bc.addInstrBool(Opcode::LOAD_DATA, stmt->getLoc(), true); return true;
    case lex::FFALSE: bc.addInstrBool(Opcode::LOAD_DATA, stmt->getLoc(), false); return true;
    case lex::NIL: bc.addInstrNil(Opcode::LOAD_DATA, stmt->getLoc()); return true;
    default: break;
    }
    err.fail(stmt->getLoc(),
             "unable to generate bytecode - unknown simple type: ", val.getTok().cStr());
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtFnArgs /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenPass::visit(StmtFnArgs *stmt, Stmt **source)
{
    // ssize_t because size_t can overflow
    for(ssize_t i = stmt->getArgs().size() - 1; i >= 0; --i) {
        Stmt *&a = stmt->getArg(i);
        if(!visit(a, &a)) {
            err.fail(a->getLoc(), "failed to generate code for function argument");
            return false;
        }
    }
    // arginfo:
    // 0 => simple
    // 1 => keyword
    // 2 => unpack
    String arginfo;
    for(size_t i = 0; i < stmt->getArgs().size(); ++i) {
        auto &a = stmt->getArg(i);
        if(a->isVar()) arginfo += '1';
        else if(stmt->unpackArg(i)) arginfo += '2';
        else arginfo += '0';
    }
    fncallarginfo.push_back(std::move(arginfo));
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtExpr /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenPass::visit(StmtExpr *stmt, Stmt **source)
{
    // if LHS is a dot operation and the current expr is a function call, we want the
    // member function call instr to be emitted
    StringRef attrname;
    // index to edit from where the jump after RHS is to occur (for && and || operations)
    size_t beforelogicaljmplocscount = jmplocs.size();
    lex::TokType oper                = stmt->getOperTok().getVal();

    size_t orInstrPos = 0;

    // handle member function call - we don't want ATTR instr to be emitted so we take care
    // of the whole thing ourselves
    if(oper == lex::FNCALL && stmt->getLHS()->isExpr()) {
        StmtExpr *l = as<StmtExpr>(stmt->getLHS());
        if(l->getOperTok().getVal() == lex::DOT) {
            assert(l->getRHS()->isSimple() && "RHS of dot operation must always be an identifier");
            if(!visit(l->getLHS(), &l->getLHS())) {
                err.fail(l->getLHS()->getLoc(),
                         "failed to generate bytecode for LHS of dot operation");
                return false;
            }
            attrname = as<StmtSimple>(l->getRHS())->getLexDataStr();
            bc.addInstrStr(Opcode::LOAD_DATA, stmt->getLoc(), attrname);
        }
    }

    if(attrname.empty() && !visit(stmt->getLHS(), &stmt->getLHS())) {
        err.fail(stmt->getLHS()->getLoc(), "failed to generate code for LHS of expression");
        return false;
    }

    if(oper == lex::OR) {
        orInstrPos = bc.size();
        bc.addInstrInt(Opcode::PUSH_TRY, stmt->getOper().getLoc(), 0); // placeholder
    }

    if(oper == lex::LAND || oper == lex::LOR) {
        jmplocs.push_back(bc.size());
        bc.addInstrInt(oper == lex::LAND ? Opcode::JMP_FALSE : Opcode::JMP_TRUE,
                       stmt->getLHS()->getLoc(), 0);
    }

    // for operator based memcall, the operator must come before RHS (AKA the memcall arg)
    if(oper != lex::ASSN && oper != lex::DOT && oper != lex::FNCALL && oper != lex::OR &&
       oper != lex::LAND && oper != lex::LOR && oper != lex::INVALID)
    {
        bc.addInstrStr(Opcode::LOAD_DATA, stmt->getOper().getLoc(), String(lex::TokStrs[oper]));
    }

    if(oper != lex::DOT && stmt->getRHS() && !visit(stmt->getRHS(), &stmt->getRHS())) {
        err.fail(stmt->getRHS()->getLoc(), "failed to generate code for RHS of expression");
        return false;
    }

    if(oper == lex::OR) {
        bc.updateInstrInt(orInstrPos, bc.size());
        bc.addInstrNil(Opcode::POP_TRY, stmt->getOper().getLoc());
        goto end;
    }

    if(oper == lex::LAND || oper == lex::LOR) {
        for(size_t i = beforelogicaljmplocscount; i < jmplocs.size(); ++i) {
            bc.updateInstrInt(jmplocs[i], bc.size());
        }
        size_t remjmps = jmplocs.size() - beforelogicaljmplocscount;
        for(size_t i = 0; i < remjmps; ++i) jmplocs.pop_back();
        goto end;
    }

    if(oper == lex::ASSN) {
        bc.addInstrNil(Opcode::STORE, stmt->getLoc());
    } else if(oper == lex::DOT) {
        assert(stmt->getRHS()->isSimple() && "RHS of dot operation must always be a primitive");
        StmtSimple *r = as<StmtSimple>(stmt->getRHS());
        if(r->getLexValue().getTok().isType(lex::INT))
            bc.addInstrStr(Opcode::ATTR, r->getLoc(),
                           std::to_string(r->getLexValue().getDataInt()));
        else bc.addInstrStr(Opcode::ATTR, r->getLoc(), r->getLexDataStr());
    } else if(oper == lex::FNCALL) {
        assert(stmt->getRHS()->isFnArgs() && "fnargs expected as RHS for function call");
        bc.addInstrStr(attrname.empty() ? Opcode::CALL : Opcode::MEM_CALL, stmt->getLoc(),
                       fncallarginfo.back());
        fncallarginfo.pop_back();
    } else {
        bc.addInstrStr(Opcode::MEM_CALL, stmt->getLoc(), StringRef(stmt->getRHS() ? "0" : ""));
    }
end:
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtVar //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenPass::visit(StmtVar *stmt, Stmt **source)
{
    Stmt *&val              = stmt->getVal();
    const lex::Lexeme &name = stmt->getName();
    if(!val && !stmt->isArg()) {
        err.fail(stmt->getLoc(),
                 "cannot generate bytecode of a variable with no value: ", name.getDataStr());
        return false;
    }
    if(val && !visit(val, &val)) {
        err.fail(stmt->getLoc(),
                 "failed to generate bytecode of variable val: ", name.getDataStr());
        return false;
    }
    if(stmt->getIn()) {
        if(!visit(stmt->getIn(), asStmt(&stmt->getIn()))) {
            err.fail(stmt->getIn()->getLoc(), "failed to generate bytecode for 'in' part");
            return false;
        }
        bc.addInstrStr(Opcode::CREATE_IN, stmt->getLoc(), name.getDataStr());
        return true;
    }
    // if the var is a function arg, CREATE instr must not be created
    if(stmt->isArg()) bc.addInstrStr(Opcode::LOAD_DATA, stmt->getLoc(), name.getDataStr());
    else bc.addInstrStr(Opcode::CREATE, stmt->getLoc(), name.getDataStr());
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFnSig ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenPass::visit(StmtFnSig *stmt, Stmt **source)
{
    String arginfo;
    arginfo += stmt->getKwArg() ? "1" : "0";
    arginfo += stmt->getVaArg() ? "1" : "0";
    Vector<StmtVar *> &args = stmt->getArgs();
    for(auto arg = args.rbegin(); arg != args.rend(); ++arg) {
        auto &a = *arg;
        if(!visit(a, asStmt(&a))) {
            err.fail(a->getLoc(), "failed to generate bytecode for function parameter: ",
                     a->getName().getDataStr());
            return false;
        }
    }
    if(stmt->getVaArg())
        bc.addInstrStr(Opcode::LOAD_DATA, stmt->getVaArg()->getLoc(),
                       stmt->getVaArg()->getLexDataStr());
    if(stmt->getKwArg())
        bc.addInstrStr(Opcode::LOAD_DATA, stmt->getKwArg()->getLoc(),
                       stmt->getKwArg()->getLexDataStr());
    for(auto &a : args) { arginfo += a->getVal() ? "1" : "0"; }
    fndefarginfo.push_back(std::move(arginfo));
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFnDef ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenPass::visit(StmtFnDef *stmt, Stmt **source)
{
    size_t blockTillPos = bc.size();
    bc.addInstrInt(Opcode::BLOCK_TILL, stmt->getLoc(), 0); // 0 is a placeholder
    if(!visit(stmt->getBlk(), asStmt(&stmt->getBlk()))) {
        err.fail(stmt->getLoc(), "failed to generate code for function definition block");
        return false;
    }
    bc.updateInstrInt(blockTillPos, bc.size() - 1);
    if(!visit(stmt->getSig(), asStmt(&stmt->getSig()))) {
        err.fail(stmt->getLoc(), "failed to generate bytecode for function signature");
        return false;
    }
    bc.addInstrStr(Opcode::CREATE_FN, stmt->getLoc(), fndefarginfo.back());
    fndefarginfo.pop_back();
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// StmtVarDecl /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenPass::visit(StmtVarDecl *stmt, Stmt **source)
{
    for(auto &d : stmt->getDecls()) {
        if(!visit(d, asStmt(&d))) return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtCond /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenPass::visit(StmtCond *stmt, Stmt **source)
{
    Vector<size_t> bodyjmps;
    for(size_t i = 0; i < stmt->getConditionals().size(); ++i) {
        auto &c            = stmt->getConditionals()[i];
        size_t falsejmppos = 0;
        if(c.getCond()) {
            if(!visit(c.getCond(), &c.getCond())) {
                err.fail(c.getCond()->getLoc(), "failed to generate code for condition");
                return false;
            }
            falsejmppos = bc.size();
            bc.addInstrInt(Opcode::JMP_FALSE_POP, c.getCond()->getLoc(), 0);
        }
        if(!visit(c.getBlk(), asStmt(&c.getBlk()))) {
            err.fail(stmt->getLoc(), "failed to generate code for conditional block");
            return false;
        }
        if(i < stmt->getConditionals().size() - 1) {
            bodyjmps.push_back(bc.size());
            bc.addInstrInt(Opcode::JMP, c.getCond() ? c.getCond()->getLoc() : c.getBlk()->getLoc(),
                           0);
        }
        if(c.getCond()) { bc.updateInstrInt(falsejmppos, bc.size()); }
    }
    for(auto &bodyjmp : bodyjmps) { bc.updateInstrInt(bodyjmp, bc.size()); }
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFor //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenPass::visit(StmtFor *stmt, Stmt **source)
{
    Stmt *&init     = stmt->getInit();
    Stmt *&cond     = stmt->getCond();
    Stmt *&incr     = stmt->getIncr();
    StmtBlock *&blk = stmt->getBlk();

    size_t condPos    = 0;
    size_t condJmpPos = 0;

    bc.addInstrNil(Opcode::PUSH_LOOP, stmt->getLoc());
    if(init) {
        if(!visit(init, &init)) {
            err.fail(init->getLoc(), "failed to generate code for loop init");
            return false;
        }
        if(init->isExpr() || init->isSimple()) {
            bc.addInstrInt(Opcode::UNLOAD, init->getLoc(), 1);
        }
    }
    condPos = bc.size();
    if(cond) {
        if(!visit(cond, &cond)) {
            err.fail(cond->getLoc(), "failed to generate code for loop init");
            return false;
        }
        condJmpPos = bc.size();
        bc.addInstrInt(Opcode::JMP_FALSE_POP, cond->getLoc(), 0); // placeholder
    }

    size_t bodyBegin = bc.size();
    if(blk && !visit(blk, asStmt(&blk))) {
        err.fail(blk->getLoc(), "failed to generate code for loop block");
        return false;
    }
    // TODO: verify this works correctly for CONTINUE since it skips over a POP_BLK;
    // the Vars (and VarStack) ::continueLoop() should be able to take care of that
    size_t bodyEnd = bc.size(); // also = continue jump location

    if(incr) {
        if(!visit(incr, &incr)) {
            err.fail(incr->getLoc(), "failed to generate code for loop init");
            return false;
        }
        if(incr->isExpr() || incr->isSimple()) {
            bc.addInstrInt(Opcode::UNLOAD, incr->getLoc(), 1);
        }
    }
    bc.addInstrInt(Opcode::JMP, stmt->getLoc(), condPos); // jmp back to condition
    if(cond) bc.updateInstrInt(condJmpPos, bc.size());

    size_t breakJmpPos = bc.size();
    bc.addInstrNil(Opcode::POP_LOOP, stmt->getLoc());

    // update all continue and break instructions
    for(size_t i = bodyBegin; i < bodyEnd; ++i) {
        auto &ins = bc.getInstrAt(i);
        if(ins.getOpcode() == Opcode::CONTINUE && ins.getDataInt() == 0) ins.setInt(bodyEnd);
        else if(ins.getOpcode() == Opcode::BREAK && ins.getDataInt() == 0) ins.setInt(breakJmpPos);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// StmtForIn /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenPass::visit(StmtForIn *stmt, Stmt **source)
{
    lex::Lexeme &iter  = stmt->getIter();
    lex::Lexeme __iter = iter;
    Stmt *&in          = stmt->getIn();
    StmtBlock *&blk    = stmt->getBlk();
    ModuleLoc loc      = stmt->getLoc();

    bc.addInstrNil(Opcode::PUSH_LOOP, loc);

    // let __<iter> = <in-expr>;
    if(!visit(in, &in)) {
        err.fail(in->getLoc(), "failed to generate bytecode for forin loop in-expr");
        return false;
    }
    __iter.setDataStr("__", __iter.getDataStr());
    bc.addInstrStr(Opcode::CREATE, loc, __iter.getDataStr());

    size_t continuejmppos = bc.size();

    // let <iter> = __<iter>.next();
    bc.addInstrIden(Opcode::LOAD_DATA, loc, __iter.getDataStr());
    bc.addInstrStr(Opcode::LOAD_DATA, loc, StringRef("next"));
    bc.addInstrStr(Opcode::MEM_CALL, loc, StringRef(""));
    // jump-nil location will be set later
    size_t jmpNilPos = bc.size();
    bc.addInstrInt(Opcode::JMP_NIL, loc, 0); // placeholder
    bc.addInstrStr(Opcode::CREATE, loc, iter.getDataStr());

    size_t bodyBegin = bc.size();
    if(blk && !visit(blk, asStmt(&blk))) {
        err.fail(blk->getLoc(), "failed to generate code for loop block");
        return false;
    }
    // TODO: verify this works correctly for CONTINUE since it skips over a POP_BLK;
    // the Vars (and VarStack) ::continueLoop() should be able to take care of that
    size_t bodyEnd = bc.size();

    bc.addInstrInt(Opcode::JMP, loc, continuejmppos);

    // this is where break jumps to
    size_t breakJmpPos = bc.size();
    bc.addInstrNil(Opcode::POP_LOOP, loc);
    // update the jump-nil loc to breakpos
    bc.updateInstrInt(jmpNilPos, breakJmpPos);

    // update all continue and break instructions
    for(size_t i = bodyBegin; i < bodyEnd; ++i) {
        auto &ins = bc.getInstrAt(i);
        if(ins.getOpcode() == Opcode::CONTINUE && ins.getDataInt() == 0) ins.setInt(continuejmppos);
        else if(ins.getOpcode() == Opcode::BREAK && ins.getDataInt() == 0) ins.setInt(breakJmpPos);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtRet //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenPass::visit(StmtRet *stmt, Stmt **source)
{
    if(stmt->getRetVal() && !visit(stmt->getRetVal(), &stmt->getRetVal())) {
        err.fail(stmt->getRetVal()->getLoc(), "failed to generate code for return value");
        return false;
    }
    bc.addInstrBool(Opcode::RETURN, stmt->getLoc(), stmt->getRetVal());
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtContinue ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenPass::visit(StmtContinue *stmt, Stmt **source)
{
    bc.addInstrInt(Opcode::CONTINUE, stmt->getLoc(), 0); // placeholder
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtBreak ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenPass::visit(StmtBreak *stmt, Stmt **source)
{
    bc.addInstrInt(Opcode::BREAK, stmt->getLoc(), 0); // placeholder
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtDefer ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CodegenPass::visit(StmtDefer *stmt, Stmt **source)
{
    err.fail(stmt->getLoc(), "defer should have been dealt with in SimplifyPass");
    return false;
}

} // namespace fer::ast