#include "AST/Parser.hpp"

namespace fer::ast
{

bool parse(ManagedList &allocator, ManagedList &toks, Stmt *&s, bool exprOnly)
{
    Parser parser(allocator, toks);
    return exprOnly ? parser.parseExpr(s) : parser.parseBlock((StmtBlock *&)s, false);
}
void dumpTree(OStream &os, Stmt *tree) { tree->disp(false); }

Parser::Parser(ManagedList &allocator, ManagedList &toks) : allocator(allocator), p(toks) {}

// on successful parse, returns true, and tree is allocated
// if withBrace is true, it will attempt to find the beginning and ending brace for each block
bool Parser::parseBlock(StmtBlock *&tree, bool withBrace)
{
    tree = nullptr;

    Vector<Stmt *> stmts;
    Stmt *stmt = nullptr;

    lex::Lexeme *start = p.peek();

    if(withBrace) {
        if(!p.acceptn(lex::LBRACE)) {
            lex::Lexeme *curr = p.peek();
            err.fail(curr->getLoc(),
                     "expected opening braces '{' for block, found: ", curr->getTok().cStr());
            return false;
        }
    }

    virtualRegisters.pushBlk();
    while(p.isValid() && (!withBrace || !p.accept(lex::RBRACE))) {
        bool skipCols = false;
        // logic
        if(p.accept(lex::LET) || (p.accept(lex::STR) && p.peekt(1) == lex::LET)) {
            if(!parseVardecl(stmt)) return false;
        } else if(p.accept(lex::IF)) {
            if(!parseConds(stmt)) return false;
            skipCols = true;
        } else if(p.accept(lex::FOR)) {
            if(p.peekt(1) == lex::IDEN && p.peekt(2) == lex::FIN) {
                if(!parseForIn(stmt)) return false;
            } else {
                if(!parseFor(stmt)) return false;
            }
            skipCols = true;
        } else if(p.accept(lex::WHILE)) {
            if(!parseWhile(stmt)) return false;
            skipCols = true;
        } else if(p.accept(lex::RETURN)) {
            if(!parseRet(stmt)) return false;
        } else if(p.accept(lex::CONTINUE)) {
            if(!parseContinue(stmt)) return false;
        } else if(p.accept(lex::BREAK)) {
            if(!parseBreak(stmt)) return false;
        } else if(p.accept(lex::DEFER)) {
            if(!parseDefer(stmt)) return false;
        } else if(p.accept(lex::INLINE)) {
            if(p.peekt(1) == lex::IF && !parseConds(stmt)) return false;
            skipCols = true;
        } else if(p.accept(lex::LBRACE)) {
            if(!parseBlock((StmtBlock *&)stmt)) return false;
            skipCols = true;
        } else if(!parseExpr(stmt)) {
            return false;
        }

        if(skipCols || p.acceptn(lex::COLS)) {
            for(auto &s : prependBlock) stmts.push_back(s);
            prependBlock.clear();
            stmts.push_back(stmt);
            stmt = nullptr;
            continue;
        }
        err.fail(p.peek()->getLoc(),
                 "expected semicolon for end of statement, found: ", p.peek()->getTok().cStr());
        return false;
    }
    virtualRegisters.popBlk();

    if(withBrace) {
        if(!p.acceptn(lex::RBRACE)) {
            err.fail(p.peek()->getLoc(),
                     "expected closing braces '}' for block, found: ", p.peek()->getTok().cStr());
            return false;
        }
    }

    tree = StmtBlock::create(allocator, start->getLoc(), std::move(stmts), !withBrace);
    return true;
}

bool Parser::parseSimple(Stmt *&data)
{
    data = nullptr;

    if(!p.peek()->getTok().isData()) {
        err.fail(p.peek()->getLoc(), "expected data here, found: ", p.peek()->getTok().cStr());
        return false;
    }

    lex::TokType ty  = p.peekt();
    lex::Lexeme *val = p.peek();
    p.next();

    if(ty == lex::IDEN) {
        size_t index = virtualRegisters.getIndex(val->getDataStr());
        data         = StmtSimple::create(allocator, val->getLoc(), ty, val->getDataStr(), index);
    } else if(ty == lex::STR) {
        data = StmtSimple::create(allocator, val->getLoc(), ty, val->getDataStr(), -1);
    } else if(ty == lex::INT) {
        data = StmtSimple::create(allocator, val->getLoc(), ty, val->getDataInt());
    } else if(ty == lex::FLT) {
        data = StmtSimple::create(allocator, val->getLoc(), ty, val->getDataFlt());
    } else {
        data = StmtSimple::create(allocator, val->getLoc(), ty, (int64_t)0);
    }

    return true;
}

// ref"Ref of this"
// 9h
// 2.5i
bool Parser::parsePrefixedSuffixedLiteral(Stmt *&expr)
{
    lex::Lexeme *iden = p.peekt() == lex::IDEN ? p.peek() : p.peek(1);
    lex::Lexeme *lit  = p.peekt() == lex::IDEN ? p.peek(1) : p.peek();

    p.next();
    p.next();

    StmtSimple *arg = nullptr;
    if(lit->getTokVal() == lex::STR) {
        arg = StmtSimple::create(allocator, lit->getLoc(), lex::STR, lit->getDataStr(), -1);
    } else if(lit->getTokVal() == lex::INT) {
        arg = StmtSimple::create(allocator, lit->getLoc(), lex::INT, lit->getDataInt());
    } else if(lit->getTokVal() == lex::FLT) {
        arg = StmtSimple::create(allocator, lit->getLoc(), lex::FLT, lit->getDataFlt());
    } else {
        err.fail(lit->getLoc(), "unexpected literal type here, found: ", lit->getTok().cStr());
        return false;
    }
    size_t index = virtualRegisters.getIndex(iden->getDataStr());
    StmtSimple *fn =
        StmtSimple::create(allocator, iden->getLoc(), lex::IDEN, iden->getDataStr(), index);
    StmtFnArgs *finfo = StmtFnArgs::create(allocator, arg->getLoc(), {arg}, {false});
    expr = StmtExpr::create(allocator, iden->getLoc(), fn, lex::TokType::FNCALL, finfo);

    return true;
}

bool Parser::parseExpr(Stmt *&expr) { return parseExpr18(expr); }

// Left Associative
// ,
bool Parser::parseExpr18(Stmt *&expr)
{
    expr = nullptr;

    Stmt *lhs = nullptr;
    Stmt *rhs = nullptr;

    lex::Lexeme *start = p.peek();
    lex::TokType oper  = lex::TokType::INVALID;

    size_t commas = 0;

    if(!parseExpr17(rhs)) { return false; }

    while(p.accept(lex::COMMA)) {
        ++commas;
        oper = p.peekt();
        p.next();
        if(!parseExpr17(lhs)) { return false; }
        rhs = StmtExpr::create(allocator, start->getLoc(), lhs, oper, rhs);
        lhs = nullptr;
    }

    expr = rhs;
    return true;
}
// Right Associative
// =
bool Parser::parseExpr17(Stmt *&expr)
{
    expr = nullptr;

    Stmt *lhs = nullptr;
    Stmt *rhs = nullptr;

    lex::TokType oper = lex::TokType::INVALID;

    lex::Lexeme *start = p.peek();

    if(!parseExpr15(rhs)) { return false; }

    while(p.accept(lex::ASSN)) {
        oper = p.peekt();
        p.next();
        if(!parseExpr15(lhs)) { return false; }
        rhs = StmtExpr::create(allocator, start->getLoc(), lhs, oper, rhs);
        lhs = nullptr;
    }

    expr = rhs;
    return true;
}
// Left Associative
// += -= *=
// /= %= <<=
// >>= &= |=
// ~= ^=
// or-block
bool Parser::parseExpr16(Stmt *&expr)
{
    expr = nullptr;

    Stmt *lhs = nullptr;
    Stmt *rhs = nullptr;

    lex::Lexeme *start = p.peek();

    if(!parseExpr14(lhs)) { return false; }

    while(p.accept(lex::ADD_ASSN, lex::SUB_ASSN, lex::MUL_ASSN) ||
          p.accept(lex::DIV_ASSN, lex::MOD_ASSN, lex::LSHIFT_ASSN) ||
          p.accept(lex::RSHIFT_ASSN, lex::BAND_ASSN, lex::BOR_ASSN) ||
          p.accept(lex::BNOT_ASSN, lex::BXOR_ASSN))
    {
        lex::TokType oper = p.peekt();
        p.next();
        if(!parseExpr14(rhs)) { return false; }
        lhs = StmtExpr::create(allocator, start->getLoc(), lhs, oper, rhs);
        rhs = nullptr;
    }

    expr = lhs;

    if(!p.accept(lex::OR)) return true;

    StmtBlock *orBlk      = nullptr;
    ModuleLoc orLoc       = p.peek()->getLoc();
    StringRef orBlkVar    = "_";
    ModuleLoc orBlkVarLoc = orLoc;
    size_t orBlkVarIndex  = -1;
    p.next();

    bool hadFunc = virtualRegisters.hasFunc();

    if(!hadFunc) virtualRegisters.pushFunc();
    size_t selfIndex = virtualRegisters.getLastIndex();
    StmtVar *selfVar =
        StmtVar::create(allocator, start->getLoc(), "self", nullptr, nullptr, selfIndex, true);

    if(p.accept(lex::IDEN)) {
        orBlkVar    = p.peek()->getDataStr();
        orBlkVarLoc = p.peek()->getLoc();
        p.next();
        virtualRegisters.pushName(orBlkVar);
        orBlkVarIndex = virtualRegisters.getLastIndex();
    }
    if(!parseBlock(orBlk)) return false;

    size_t reqdRegisters = 0;
    if(!hadFunc) reqdRegisters = virtualRegisters.popFunc();

    ensureBlockReturns(orBlk);

    StmtVar *arg =
        StmtVar::create(allocator, orBlkVarLoc, orBlkVar, nullptr, nullptr, orBlkVarIndex, true);
    StmtFnSig *fnsig =
        StmtFnSig::create(allocator, orBlkVarLoc, {selfVar, arg}, nullptr, nullptr, true);
    StmtFnDef *fndef =
        StmtFnDef::create(allocator, orBlkVarLoc, fnsig, orBlk, reqdRegisters, orBlkVarIndex);

    // expr with or blk's format is: <fndef> <OR> <expr>
    expr = StmtExpr::create(allocator, orLoc, fndef, lex::OR, expr);
    return true;
}
// Left Associative
// ?:
bool Parser::parseExpr15(Stmt *&expr)
{
    expr = nullptr;

    Vector<Conditional> cvec;
    Stmt *cond    = nullptr;
    Stmt *blkStmt = nullptr;

    lex::Lexeme *start = p.peek();

    if(!parseExpr16(cond)) { return false; }
    if(!p.acceptn(lex::QUEST)) {
        expr = cond;
        return true;
    }

    if(!parseExpr16(blkStmt)) { return false; }
    if(!p.acceptn(lex::COL)) {
        err.fail(p.peek()->getLoc(),
                 "expected ':' for ternary operator, found: ", p.peek()->getTok().cStr());
        return false;
    }
    blkStmt = StmtBlock::create(allocator, blkStmt->getLoc(), {blkStmt}, true);
    as<StmtBlock>(blkStmt)->setUnload(false);
    cvec.emplace_back(cond, as<StmtBlock>(blkStmt));
    blkStmt = nullptr;
    if(!parseExpr16(blkStmt)) { return false; }
    blkStmt = StmtBlock::create(allocator, blkStmt->getLoc(), {blkStmt}, true);
    as<StmtBlock>(blkStmt)->setUnload(false);
    cvec.emplace_back(nullptr, as<StmtBlock>(blkStmt));
    expr = StmtCond::create(allocator, cond->getLoc(), cvec);
    return true;
}
// Left Associative
// ??
bool Parser::parseExpr14(Stmt *&expr)
{
    expr = nullptr;

    Stmt *lhs = nullptr;
    Stmt *rhs = nullptr;

    lex::Lexeme *start = p.peek();
    lex::TokType oper  = lex::TokType::INVALID;

    if(!parseExpr13(lhs)) { return false; }

    while(p.accept(lex::NIL_COALESCE)) {
        oper = p.peekt();
        p.next();
        if(!parseExpr13(rhs)) { return false; }
        lhs = StmtExpr::create(allocator, start->getLoc(), lhs, oper, rhs);
        rhs = nullptr;
    }

    expr = lhs;
    return true;
}
// Left Associative
// ||
bool Parser::parseExpr13(Stmt *&expr)
{
    expr = nullptr;

    Stmt *lhs = nullptr;
    Stmt *rhs = nullptr;

    lex::Lexeme *start = p.peek();
    lex::TokType oper  = lex::TokType::INVALID;

    if(!parseExpr12(lhs)) { return false; }

    while(p.accept(lex::LOR)) {
        oper = p.peekt();
        p.next();
        if(!parseExpr12(rhs)) { return false; }
        lhs = StmtExpr::create(allocator, start->getLoc(), lhs, oper, rhs);
        rhs = nullptr;
    }

    expr = lhs;
    return true;
}
// Left Associative
// &&
bool Parser::parseExpr12(Stmt *&expr)
{
    expr = nullptr;

    Stmt *lhs = nullptr;
    Stmt *rhs = nullptr;

    lex::Lexeme *start = p.peek();
    lex::TokType oper  = lex::TokType::INVALID;

    if(!parseExpr11(lhs)) { return false; }

    while(p.accept(lex::LAND)) {
        oper = p.peekt();
        p.next();
        if(!parseExpr11(rhs)) { return false; }
        lhs = StmtExpr::create(allocator, start->getLoc(), lhs, oper, rhs);
        rhs = nullptr;
    }

    expr = lhs;
    return true;
}
// Left Associative
// |
bool Parser::parseExpr11(Stmt *&expr)
{
    expr = nullptr;

    Stmt *lhs = nullptr;
    Stmt *rhs = nullptr;

    lex::Lexeme *start = p.peek();
    lex::TokType oper  = lex::TokType::INVALID;

    if(!parseExpr10(lhs)) { return false; }

    while(p.accept(lex::BOR)) {
        oper = p.peekt();
        p.next();
        if(!parseExpr10(rhs)) { return false; }
        lhs = StmtExpr::create(allocator, start->getLoc(), lhs, oper, rhs);
        rhs = nullptr;
    }

    expr = lhs;
    return true;
}
// Left Associative
// ^
bool Parser::parseExpr10(Stmt *&expr)
{
    expr = nullptr;

    Stmt *lhs = nullptr;
    Stmt *rhs = nullptr;

    lex::Lexeme *start = p.peek();
    lex::TokType oper  = lex::TokType::INVALID;

    if(!parseExpr09(lhs)) { return false; }

    while(p.accept(lex::BXOR)) {
        oper = p.peekt();
        p.next();
        if(!parseExpr09(rhs)) { return false; }
        lhs = StmtExpr::create(allocator, start->getLoc(), lhs, oper, rhs);
        rhs = nullptr;
    }

    expr = lhs;
    return true;
}
// Left Associative
// &
bool Parser::parseExpr09(Stmt *&expr)
{
    expr = nullptr;

    Stmt *lhs = nullptr;
    Stmt *rhs = nullptr;

    lex::Lexeme *start = p.peek();
    lex::TokType oper  = lex::TokType::INVALID;

    if(!parseExpr08(lhs)) { return false; }

    while(p.accept(lex::BAND)) {
        oper = p.peekt();
        p.next();
        if(!parseExpr08(rhs)) { return false; }
        lhs = StmtExpr::create(allocator, start->getLoc(), lhs, oper, rhs);
        rhs = nullptr;
    }

    expr = lhs;
    return true;
}
// Left Associative
// == !=
bool Parser::parseExpr08(Stmt *&expr)
{
    expr = nullptr;

    Stmt *lhs = nullptr;
    Stmt *rhs = nullptr;

    lex::Lexeme *start = p.peek();
    lex::TokType oper  = lex::TokType::INVALID;

    if(!parseExpr07(lhs)) { return false; }

    while(p.accept(lex::EQ, lex::NE)) {
        oper = p.peekt();
        p.next();
        if(!parseExpr07(rhs)) { return false; }
        lhs = StmtExpr::create(allocator, start->getLoc(), lhs, oper, rhs);
        rhs = nullptr;
    }

    expr = lhs;
    return true;
}
// Left Associative
// < <=
// > >=
bool Parser::parseExpr07(Stmt *&expr)
{
    expr = nullptr;

    Stmt *lhs = nullptr;
    Stmt *rhs = nullptr;

    lex::Lexeme *start = p.peek();
    lex::TokType oper  = lex::TokType::INVALID;

    if(!parseExpr06(lhs)) { return false; }

    while(p.accept(lex::LT, lex::LE) || p.accept(lex::GT, lex::GE)) {
        oper = p.peekt();
        p.next();
        if(!parseExpr06(rhs)) { return false; }
        lhs = StmtExpr::create(allocator, start->getLoc(), lhs, oper, rhs);
        rhs = nullptr;
    }

    expr = lhs;
    return true;
}
// Left Associative
// << >>
bool Parser::parseExpr06(Stmt *&expr)
{
    expr = nullptr;

    Stmt *lhs = nullptr;
    Stmt *rhs = nullptr;

    lex::Lexeme *start = p.peek();
    lex::TokType oper  = lex::TokType::INVALID;

    if(!parseExpr05(lhs)) { return false; }

    while(p.accept(lex::LSHIFT, lex::RSHIFT)) {
        oper = p.peekt();
        p.next();
        if(!parseExpr05(rhs)) { return false; }
        lhs = StmtExpr::create(allocator, start->getLoc(), lhs, oper, rhs);
        rhs = nullptr;
    }

    expr = lhs;
    return true;
}
// Left Associative
// + -
bool Parser::parseExpr05(Stmt *&expr)
{
    expr = nullptr;

    Stmt *lhs = nullptr;
    Stmt *rhs = nullptr;

    lex::Lexeme *start = p.peek();
    lex::TokType oper  = lex::TokType::INVALID;

    if(!parseExpr04(lhs)) { return false; }

    while(p.accept(lex::ADD, lex::SUB)) {
        oper = p.peekt();
        p.next();
        if(!parseExpr04(rhs)) { return false; }
        lhs = StmtExpr::create(allocator, start->getLoc(), lhs, oper, rhs);
        rhs = nullptr;
    }

    expr = lhs;
    return true;
}
// Left Associative
// * / % ** //
bool Parser::parseExpr04(Stmt *&expr)
{
    expr = nullptr;

    Stmt *lhs = nullptr;
    Stmt *rhs = nullptr;

    lex::Lexeme *start = p.peek();
    lex::TokType oper  = lex::TokType::INVALID;

    if(!parseExpr03(lhs)) { return false; }

    while(p.accept(lex::MUL, lex::DIV, lex::MOD) || p.accept(lex::POWER, lex::ROOT)) {
        oper = p.peekt();
        p.next();
        if(!parseExpr03(rhs)) { return false; }
        lhs = StmtExpr::create(allocator, start->getLoc(), lhs, oper, rhs);
        rhs = nullptr;
    }

    expr = lhs;
    return true;
}
// Right Associative (single operand)
// ++ -- (pre)
// + - (unary)
// * & (deref, addrof)
// ! ~ (log/bit)
bool Parser::parseExpr03(Stmt *&expr)
{
    expr = nullptr;

    Stmt *lhs = nullptr;

    lex::Lexeme *start = p.peek();
    Vector<lex::TokType> opers;

    while(p.accept(lex::XINC, lex::XDEC) || p.accept(lex::ADD, lex::SUB) ||
          p.accept(lex::MUL, lex::BAND) || p.accept(lex::LNOT, lex::BNOT))
    {
        if(p.peekt() == lex::XINC) p.sett(lex::INCX);
        if(p.peekt() == lex::XDEC) p.sett(lex::DECX);
        if(p.peekt() == lex::ADD) p.sett(lex::UADD);
        if(p.peekt() == lex::SUB) p.sett(lex::USUB);
        if(p.peekt() == lex::MUL) p.sett(lex::UMUL);
        if(p.peekt() == lex::BAND) p.sett(lex::UAND);
        opers.insert(opers.begin(), p.peekt());
        p.next();
    }

    if(!parseExpr02(lhs)) { return false; }

    if(!lhs) {
        err.fail(start->getLoc(), "invalid expression");
        return false;
    }

    if(lhs->isSimple() && !opers.empty()) {
        StmtSimple *l   = as<StmtSimple>(lhs);
        lex::TokType tk = l->getTokType();
        if(tk == lex::INT) {
            while(!opers.empty() && opers.front() == lex::USUB) {
                l->setData(-l->getDataInt());
                opers.erase(opers.begin());
            }
        }
        if(tk == lex::FLT) {
            while(!opers.empty() && opers.front() == lex::USUB) {
                l->setData(-l->getDataFlt());
                opers.erase(opers.begin());
            }
        }
    }

    for(auto &op : opers) { lhs = StmtExpr::create(allocator, start->getLoc(), lhs, op, nullptr); }

    expr = lhs;
    return true;
}
// Left Associative
// ++ -- (post)
// ... (postva)
bool Parser::parseExpr02(Stmt *&expr)
{
    expr = nullptr;

    Stmt *lhs = nullptr;

    lex::Lexeme *start = p.peek();

    if(!parseExpr01(lhs)) { return false; }

    if(p.accept(lex::XINC, lex::XDEC, lex::PreVA)) {
        if(p.accept(lex::PreVA)) p.sett(lex::PostVA);
        lhs = StmtExpr::create(allocator, p.peek()->getLoc(), lhs, p.peekt(), nullptr);
        p.next();
    }

    expr = lhs;
    return true;
}
bool Parser::parseExpr01(Stmt *&expr)
{
    expr = nullptr;

    lex::Lexeme *start = p.peek();
    ModuleLoc dotLoc;

    Stmt *lhs = nullptr;
    Stmt *rhs = nullptr;
    Vector<Stmt *> args;
    Stmt *arg = nullptr;
    Vector<bool> unpackVector; // works for variadic as well

    // prefixed/suffixed literals
    if(p.accept(lex::IDEN) && p.peek(1)->getTok().isLiteral() ||
       p.peek()->getTok().isLiteral() && p.peekt(1) == lex::IDEN)
    {
        return parsePrefixedSuffixedLiteral(expr);
    }

    if(p.acceptn(lex::LPAREN)) {
        if(!parseExpr(lhs)) { return false; }
        if(!p.acceptn(lex::RPAREN)) {
            err.fail(p.peek()->getLoc(), "expected ending parenthesis ')' for expression, found: ",
                     p.peek()->getTok().cStr());
            return false;
        }
    }

    if(p.acceptd() && !parseSimple(lhs)) {
        err.fail(p.peek()->getLoc(), "failed to parse simple");
        return false;
    }
    if(p.accept(lex::FN) && !parseFnDef(lhs)) return false;
    if(p.accept(lex::AWAIT, lex::WAIT) && !parseAwait(lhs)) return false;
    if(p.accept(lex::YIELD) && !parseRet(lhs)) return false;
    goto beginBrack;

afterDot:
    if(!p.acceptd() || !parseSimple(rhs)) return false;
    if(lhs && rhs) {
        lhs    = StmtExpr::create(allocator, dotLoc, lhs, lex::DOT, rhs);
        rhs    = nullptr;
        dotLoc = ModuleLoc();
    }

beginBrack:
    if(p.accept(lex::LBRACK)) {
        p.sett(lex::SUBS);
        lex::Lexeme *oper = p.peek();
        p.next();
        if(!parseExpr17(rhs)) {
            err.fail(oper->getLoc(), "failed to parse expression for subscript");
            return false;
        }
        if(!p.acceptn(lex::RBRACK)) {
            err.fail(p.peek()->getLoc(),
                     "expected closing bracket for"
                     " subscript expression, found: ",
                     p.peek()->getTok().cStr());
            return false;
        }
        lhs = StmtExpr::create(allocator, oper->getLoc(), lhs, oper->getTokVal(), rhs);
        rhs = nullptr;
        if(p.accept(lex::LBRACK, lex::LPAREN) || (p.peekt() == lex::DOT && p.peekt(1) == lex::LT))
            goto beginBrack;
    } else if(p.accept(lex::LPAREN)) {
        p.sett(lex::FNCALL);
        lex::Lexeme *oper = p.peek();
        p.next();
        if(p.acceptn(lex::RPAREN)) goto postArgs;
        // parse arguments
        while(true) {
            if(p.accept(lex::STR, lex::IDEN) && p.peekt(1) == lex::ASSN) {
                // assn args (begins with <STR/IDEN> '=')
                if(p.accept(lex::IDEN)) p.sett(lex::STR);
                lex::Lexeme *name = p.peek();
                p.next();
                p.next();
                if(!parseExpr17(arg)) return false;
                arg = StmtVar::create(allocator, name->getLoc(), name->getDataStr(), nullptr, arg,
                                      -1, true);
            } else if(!parseExpr17(arg)) { // normal arg
                return false;
            }
            unpackVector.push_back(arg->isExpr() && as<StmtExpr>(arg)->isOper(lex::PostVA));
            if(unpackVector.back()) arg = as<StmtExpr>(arg)->getLHS();
            args.push_back(arg);
            arg = nullptr;
            if(!p.acceptn(lex::COMMA)) break;
        }
        if(!p.acceptn(lex::RPAREN)) {
            err.fail(p.peek()->getLoc(),
                     "expected closing parenthesis/brace after "
                     "a call arguments, found: ",
                     p.peek()->getTok().cStr());
            return false;
        }
    postArgs:
        rhs =
            StmtFnArgs::create(allocator, oper->getLoc(), std::move(args), std::move(unpackVector));
        lhs          = StmtExpr::create(allocator, oper->getLoc(), lhs, oper->getTokVal(), rhs);
        rhs          = nullptr;
        args         = {};
        unpackVector = {};

        if(p.accept(lex::LBRACK, lex::LPAREN)) goto beginBrack;
    }

    if(p.acceptn(lex::DOT, lex::ARROW)) {
        dotLoc = p.peek(-1)->getLoc();
        if(lhs && rhs) {
            lhs = StmtExpr::create(allocator, dotLoc, lhs, lex::DOT, rhs);
            rhs = nullptr;
        }
        goto afterDot;
    }

    if(lhs && rhs) {
        lhs = StmtExpr::create(allocator, dotLoc, lhs, lex::DOT, rhs);
        rhs = nullptr;
    }
    expr = lhs;
    return true;
}

bool Parser::parseVar(StmtVar *&var, bool isFnArg)
{
    var = nullptr;

    if(!p.accept(lex::IDEN, lex::STR)) {
        err.fail(p.peek()->getLoc(),
                 "expected identifier for variable name, found: ", p.peek()->getTok().cStr());
        return false;
    }
    lex::Lexeme *start = p.peek();
    StringRef name     = start->getDataStr();
    p.next();
    Stmt *val = nullptr;
    Stmt *in  = nullptr;

    if(p.acceptn(lex::FIN) && !isFnArg) {
        if(!parseExpr01((Stmt *&)in)) {
            err.fail(p.peek()->getLoc(), "failed to parse in-type for variable: ", name);
            return false;
        }
    }

    if(!p.acceptn(lex::ASSN)) {
        if(isFnArg) goto end;
        err.fail(start->getLoc(), "invalid variable declaration - no value set");
        return false;
    }
    if(!parseExpr17(val)) return false;

end:
    size_t index = -1;
    if(!in) {
        virtualRegisters.pushName(name);
        index = virtualRegisters.getLastIndex();
    }
    var = StmtVar::create(allocator, start->getLoc(), name, in, val, index, isFnArg);
    return true;
}

bool Parser::parseFnSig(Stmt *&fsig)
{
    fsig = nullptr;

    Vector<StmtVar *> args;
    StmtVar *arg = nullptr;
    Set<StringRef> argnames;
    StmtSimple *kwArg = nullptr, *vaarg = nullptr;
    lex::Lexeme *start = p.peek();

    if(!p.acceptn(lex::FN)) {
        err.fail(p.peek()->getLoc(), "expected 'fn' here, found: ", p.peek()->getTok().cStr());
        return false;
    }

    if(!p.acceptn(lex::LPAREN)) {
        err.fail(p.peek()->getLoc(), "expected opening parenthesis for function args, found: ",
                 p.peek()->getTok().cStr());
        return false;
    }

    size_t selfIndex = virtualRegisters.getLastIndex();
    StmtVar *selfVar =
        StmtVar::create(allocator, start->getLoc(), "self", nullptr, nullptr, selfIndex, true);
    argnames.insert("self");
    args.push_back(selfVar);

    if(p.acceptn(lex::RPAREN)) goto postArgs;

    // args
    while(true) {
        bool tryKW = false;
        if(p.accept(lex::STR)) tryKW = true;
        if(!p.accept(lex::IDEN, lex::STR)) {
            err.fail(p.peek()->getLoc(),
                     "expected identifier/str for argument, found: ", p.peek()->getTok().cStr());
            return false;
        }
        StringRef name = p.peek()->getDataStr();
        if(argnames.find(name) != argnames.end()) {
            err.fail(p.peek()->getLoc(), "this argument name is already used "
                                         "before in this function signature");
            return false;
        }
        argnames.insert(name);
        // this is a keyword arg
        if(tryKW) {
            if(kwArg) {
                err.fail(p.peek()->getLoc(),
                         "function cannot have multiple"
                         " keyword arguments (previous: ",
                         kwArg->getDataStr(), ")");
                return false;
            }
            virtualRegisters.pushName(name);
            size_t index = virtualRegisters.getLastIndex();
            kwArg        = StmtSimple::create(allocator, p.peek()->getLoc(), lex::STR, name, index);
            p.next();
        } else if(p.peekt(1) == lex::PreVA) {
            p.peek(1)->getTok().setVal(lex::PostVA);
            // no check for multiple variadic as no arg can exist after a variadic
            virtualRegisters.pushName(name);
            size_t index = virtualRegisters.getLastIndex();
            vaarg = StmtSimple::create(allocator, p.peek()->getLoc(), lex::IDEN, name, index);
            p.next();
            p.next();
        } else {
            if(!parseVar(arg, true)) {
                err.fail(p.peek()->getLoc(), "failed to parse function definition parameter");
                return false;
            }
            args.push_back(arg);
            arg = nullptr;
        }
        if(!p.acceptn(lex::COMMA)) break;
        if(vaarg) {
            err.fail(p.peek()->getLoc(), "no parameter can exist after variadic");
            return false;
        }
    }

    if(!p.acceptn(lex::RPAREN)) {
        err.fail(p.peek()->getLoc(), "expected closing parenthesis after function args, found: ",
                 p.peek()->getTok().cStr());
        return false;
    }

postArgs:
    fsig = StmtFnSig::create(allocator, start->getLoc(), args, kwArg, vaarg, false);
    return true;
}
bool Parser::parseFnDef(Stmt *&fndef)
{
    fndef = nullptr;

    Stmt *sig          = nullptr;
    StmtBlock *blk     = nullptr;
    lex::Lexeme *start = p.peek();

    virtualRegisters.pushFunc();
    if(!parseFnSig(sig)) return false;
    if(!parseBlock(blk)) return false;
    size_t selfIndex     = virtualRegisters.getIndex("self");
    size_t reqdRegisters = virtualRegisters.popFunc();

    ensureBlockReturns(blk);

    fndef = StmtFnDef::create(allocator, start->getLoc(), (StmtFnSig *)sig, blk, reqdRegisters,
                              selfIndex + 1);
    return true;
}

bool Parser::parseVardecl(Stmt *&vd)
{
    vd = nullptr;

    Vector<StmtVar *> decls;
    StmtVar *decl = nullptr;
    StringRef doc = "";
    if(p.accept(lex::STR)) {
        doc = p.peek()->getDataStr();
        p.next();
    }
    lex::Lexeme *start = p.peek();

    if(!p.acceptn(lex::LET)) {
        err.fail(p.peek()->getLoc(),
                 "expected 'let' keyword here, found: ", p.peek()->getTok().cStr());
        return false;
    }

    while(p.accept(lex::IDEN, lex::STR)) {
        if(!parseVar(decl, false)) return false;
        if(!doc.empty()) decl->setDoc(doc);
        decls.push_back(decl);
        decl = nullptr;
        if(!p.acceptn(lex::COMMA)) break;
    }

    vd = StmtVarDecl::create(allocator, start->getLoc(), decls);
    return true;
}

bool Parser::parseConds(Stmt *&conds)
{
    conds = nullptr;

    Vector<Conditional> cvec;
    Conditional c(nullptr, nullptr);

    lex::Lexeme *start = p.peek();

    bool isInline = p.acceptn(lex::INLINE);

cond:
    if(!p.acceptn(lex::IF, lex::ELIF)) {
        err.fail(p.peek()->getLoc(), "expected 'if' here, found: ", p.peek()->getTok().cStr());
        return false;
    }

    if(!parseExpr16(c.getCond())) {
        err.fail(p.peek()->getLoc(), "failed to parse condition for if/else if statement");
        return false;
    }

blk:
    if(!parseBlock(c.getBlk())) {
        err.fail(p.peek()->getLoc(), "failed to parse block for conditional");
        return false;
    }
    // If the conditional is inline, the block shouldn't generate PUSH/POP_BLK instructions.
    if(isInline) c.getBlk()->setTop(isInline);

    cvec.emplace_back(c.getCond(), c.getBlk());
    c.reset();

    if(p.accept(lex::ELIF)) goto cond;
    if(p.acceptn(lex::ELSE)) goto blk;

    conds = StmtCond::create(allocator, start->getLoc(), cvec);
    return true;
}
// For-In transformation:
//
// for e in vec.eachRev() {
//     ...
// }
// ----------------------
// will generate
// ----------------------
// LOOP_BEGIN
// let __e = vec.eachRev();
// INIT:
// let x = __e.next();
// {
//     if x is nil, jump to LOOP_END (done using JMP_NIL instr)
//     ...
// }
// JMP INIT
// LOOP_END
bool Parser::parseForIn(Stmt *&fin)
{
    fin = nullptr;

    Stmt *in           = nullptr; // Expr15
    StmtBlock *blk     = nullptr;
    lex::Lexeme *start = p.peek();

    if(!p.acceptn(lex::FOR)) {
        err.fail(p.peek()->getLoc(), "expected 'for' here, found: ", p.peek()->getTok().cStr());
        return false;
    }

    if(!p.accept(lex::IDEN)) {
        err.fail(p.peek()->getLoc(),
                 "expected iterator (identifier) here, found: ", p.peek()->getTok().cStr());
        return false;
    }
    lex::Lexeme *iter = p.peek();
    virtualRegisters.pushName(iter->getDataStr());
    p.next();

    if(!p.acceptn(lex::FIN)) {
        err.fail(p.peek()->getLoc(), "expected 'in' here, found: ", p.peek()->getTok().cStr());
        return false;
    }

    if(!parseExpr16(in)) {
        err.fail(p.peek()->getLoc(), "failed to parse expression for 'in'");
        return false;
    }

    if(!p.accept(lex::LBRACE)) {
        err.fail(p.peek()->getLoc(),
                 "expected block for for-in construct, found: ", p.peek()->getTok().cStr());
        return false;
    }

    if(!parseBlock(blk)) {
        err.fail(p.peek()->getLoc(), "failed to parse block for for-in construct");
        return false;
    }

    fin = StmtForIn::create(allocator, start->getLoc(), iter->getDataStr(), in, blk);
    return true;
}
bool Parser::parseFor(Stmt *&f)
{
    f = nullptr;

    Stmt *init         = nullptr; // either of StmtVarDecl or StmtExpr
    Stmt *cond         = nullptr;
    Stmt *incr         = nullptr;
    StmtBlock *blk     = nullptr;
    lex::Lexeme *start = p.peek();

    if(!p.acceptn(lex::FOR)) {
        err.fail(p.peek()->getLoc(), "expected 'for' here, found: ", p.peek()->getTok().cStr());
        return false;
    }

    if(p.acceptn(lex::COLS)) goto cond;

    if(p.accept(lex::LET)) {
        if(!parseVardecl(init)) return false;
    } else {
        if(!parseExpr(init)) return false;
    }
    if(!p.acceptn(lex::COLS)) {
        err.fail(p.peek()->getLoc(), "expected semicolon here, found: ", p.peek()->getTok().cStr());
        return false;
    }

cond:
    if(p.acceptn(lex::COLS)) goto incr;

    if(!parseExpr17(cond)) return false;
    if(!p.acceptn(lex::COLS)) {
        err.fail(p.peek()->getLoc(), "expected semicolon here, found: ", p.peek()->getTok().cStr());
        return false;
    }

incr:
    if(p.accept(lex::LBRACE)) goto body;

    if(!parseExpr(incr)) return false;
    if(!p.accept(lex::LBRACE)) {
        err.fail(p.peek()->getLoc(),
                 "expected braces for body here, found: ", p.peek()->getTok().cStr());
        return false;
    }

body:
    if(!parseBlock(blk)) {
        err.fail(p.peek()->getLoc(), "failed to parse block for 'for' construct");
        return false;
    }

    f = StmtFor::create(allocator, start->getLoc(), init, cond, incr, blk);
    return true;
}
bool Parser::parseWhile(Stmt *&w)
{
    w = nullptr;

    Stmt *cond         = nullptr;
    StmtBlock *blk     = nullptr;
    lex::Lexeme *start = p.peek();

    if(!p.acceptn(lex::WHILE)) {
        err.fail(p.peek()->getLoc(), "expected 'while' here, found: ", p.peek()->getTok().cStr());
        return false;
    }

    if(!parseExpr17(cond)) return false;

    if(!parseBlock(blk)) {
        err.fail(p.peek()->getLoc(), "failed to parse block for 'for' construct");
        return false;
    }

    w = StmtFor::create(allocator, start->getLoc(), nullptr, cond, nullptr, blk);
    return true;
}
bool Parser::parseRet(Stmt *&ret)
{
    ret = nullptr;

    Stmt *val          = nullptr;
    lex::Lexeme *start = p.peek();

    if(!p.acceptn(lex::RETURN, lex::YIELD)) {
        err.fail(p.peek()->getLoc(),
                 "expected 'return' / 'yield' here, found: ", p.peek()->getTok().cStr());
        return false;
    }

    if(p.accept(lex::COLS)) goto done;

    if(!parseExpr17(val)) {
        lex::Lexeme *l = p.isValid() ? p.peek() : start;
        err.fail(l->getLoc(), "failed to parse expression for return value");
        return false;
    }

done:
    ret = StmtRetYield::create(allocator, start->getLoc(), val, start->getTok().isType(lex::YIELD));
    return true;
}
bool Parser::parseContinue(Stmt *&cont)
{
    cont = nullptr;

    lex::Lexeme *start = p.peek();

    if(!p.acceptn(lex::CONTINUE)) {
        err.fail(p.peek()->getLoc(),
                 "expected 'continue' here, found: ", p.peek()->getTok().cStr());
        return false;
    }

    cont = StmtContinue::create(allocator, start->getLoc());
    return true;
}
bool Parser::parseBreak(Stmt *&brk)
{
    brk = nullptr;

    lex::Lexeme *start = p.peek();

    if(!p.acceptn(lex::BREAK)) {
        err.fail(p.peek()->getLoc(), "expected 'break' here, found: ", p.peek()->getTok().cStr());
        return false;
    }

    brk = StmtBreak::create(allocator, start->getLoc());
    return true;
}
bool Parser::parseDefer(Stmt *&defer)
{
    defer = nullptr;

    Stmt *val          = nullptr;
    lex::Lexeme *start = p.peek();

    if(!p.acceptn(lex::DEFER)) {
        err.fail(p.peek()->getLoc(), "expected 'defer' here, found: ", p.peek()->getTok().cStr());
        return false;
    }

    if(!parseExpr17(val)) {
        err.fail(p.peek()->getLoc(), "failed to parse expression for defer value");
        return false;
    }

done:
    defer = StmtDefer::create(allocator, start->getLoc(), val);
    return true;
}

void Parser::ensureBlockReturns(StmtBlock *blk)
{
    auto &stmts = blk->getStmts();
    if(stmts.empty() || !stmts.back()->isReturn()) {
        stmts.emplace_back(StmtRetYield::create(allocator, blk->getLoc(), nullptr, false));
    }
}

/*
... await <expr>(args...) ...

expands to

let __futureVar<N>__ = async(<expr>, args...);
while !__futureVar<N>__.done() {
    yield __futureVar<N>__();
}
... __futureVar<N>__.result() ...
*/
bool Parser::parseAwait(Stmt *&resultCallExpr)
{
    lex::Lexeme *start = p.peek();
    ModuleLoc loc      = start->getLoc();

    static size_t varCtr = 0;

    if(!p.acceptn(lex::AWAIT, lex::WAIT)) {
        err.fail(loc, "expected `await` / `wait` here, found: ", p.peek()->getTok().cStr());
        return false;
    }
    Stmt *val = nullptr;
    if(!parseExpr01(val)) {
        err.fail(loc, "failed to parse await expression");
        return false;
    }

    if(!val->isExpr() || !as<StmtExpr>(val)->isOper(lex::FNCALL)) {
        err.fail(loc, "expected await to invoke a function call in the expression");
        return false;
    }
    Stmt *valExpr = as<StmtExpr>(val)->getLHS();
    if(!as<StmtExpr>(val)->getRHS() || !as<StmtExpr>(val)->getRHS()->isFnArgs()) {
        err.fail(loc, "expected await expression to be a function call");
        return false;
    }

    bool isWait = start->getTok().isType(lex::WAIT);

    // async(<expr>, args...)
    StmtFnArgs *valExprArgs = as<StmtFnArgs>(as<StmtExpr>(val)->getRHS());
    valExprArgs->insertArg(0, valExpr, false);
    StmtSimple *asyncLHS = StmtSimple::create(allocator, loc, lex::IDEN, StringRef("async"), -1);
    StmtExpr *asyncCall  = StmtExpr::create(allocator, loc, asyncLHS, lex::FNCALL, valExprArgs);

    // let __futureVar<N>__ = async(<expr>, args...);
    String futureVarName  = "__futureVar" + std::to_string(varCtr++) + "__";
    size_t futureVarIndex = -1;
    if(virtualRegisters.pushName(futureVarName)) futureVarIndex = virtualRegisters.getLastIndex();
    StmtVar *futureVar =
        StmtVar::create(allocator, loc, futureVarName, nullptr, asyncCall, futureVarIndex, false);
    prependBlock.push_back(futureVar);

    // while !__futureVar<N>__.done() {
    //     yield __futureVar<N>__();
    // }
    StmtSimple *futureDoneSimple =
        StmtSimple::create(allocator, loc, lex::IDEN, futureVarName, futureVarIndex);
    StmtSimple *doneRHS = StmtSimple::create(allocator, loc, lex::STR, StringRef("done"), -1);
    StmtExpr *doneExpr  = StmtExpr::create(allocator, loc, futureDoneSimple, lex::DOT, doneRHS);
    StmtFnArgs *args    = StmtFnArgs::create(allocator, loc, {}, {});
    StmtExpr *doneCall  = StmtExpr::create(allocator, loc, doneExpr, lex::FNCALL, args);
    StmtExpr *notDone   = StmtExpr::create(allocator, loc, doneCall, lex::LNOT, nullptr);

    StmtSimple *futureCallSimple =
        StmtSimple::create(allocator, loc, lex::IDEN, futureVarName, futureVarIndex);
    StmtExpr *callFuture = StmtExpr::create(allocator, loc, futureCallSimple, lex::FNCALL, args);
    Stmt *blkStmt        = nullptr;
    if(isWait) {
        blkStmt = callFuture;
    } else {
        blkStmt = StmtRetYield::create(allocator, loc, callFuture, true);
    }
    StmtBlock *blk = StmtBlock::create(allocator, loc, {blkStmt}, false);

    StmtFor *loop = StmtFor::create(allocator, loc, nullptr, notDone, nullptr, blk);

    prependBlock.push_back(loop);

    // ... __futureVar<N>__.result() ...
    StmtSimple *futureResultSimple =
        StmtSimple::create(allocator, loc, lex::IDEN, futureVarName, futureVarIndex);
    StmtSimple *resultRHS = StmtSimple::create(allocator, loc, lex::STR, StringRef("result"), -1);
    StmtExpr *resultExpr =
        StmtExpr::create(allocator, loc, futureResultSimple, lex::DOT, resultRHS);

    resultCallExpr = StmtExpr::create(allocator, loc, resultExpr, lex::FNCALL, args);
    return true;
}

} // namespace fer::ast
