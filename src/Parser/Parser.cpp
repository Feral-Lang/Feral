#include "AST/Parser.hpp"

namespace fer::ast
{

bool parse(ManagedAllocator &allocator, Vector<lex::Lexeme> &toks, Stmt *&s, bool exprOnly)
{
	Parser parser(allocator, toks);
	return exprOnly ? parser.parseExpr(s) : parser.parseBlock((StmtBlock *&)s, false);
}
void dumpTree(OStream &os, Stmt *tree) { tree->disp(false); }

Parser::Parser(ManagedAllocator &allocator, Vector<lex::Lexeme> &toks)
	: allocator(allocator), p(toks)
{}

// on successful parse, returns true, and tree is allocated
// if with_brace is true, it will attempt to find the beginning and ending brace for each block
bool Parser::parseBlock(StmtBlock *&tree, bool with_brace)
{
	tree = nullptr;

	Vector<Stmt *> stmts;
	Stmt *stmt = nullptr;

	lex::Lexeme &start = p.peek();

	if(with_brace) {
		if(!p.acceptn(lex::LBRACE)) {
			err.fail(
			p.peek().getLoc(),
			"expected opening braces '{' for block, found: ", p.peek().getTok().cStr());
			return false;
		}
	}

	while(p.isValid() && (!with_brace || !p.accept(lex::RBRACE))) {
		bool skip_cols = false;
		// logic
		if(p.accept(lex::LET)) {
			if(!parseVardecl(stmt)) return false;
		} else if(p.accept(lex::IF)) {
			if(!parseConds(stmt)) return false;
			skip_cols = true;
		} else if(p.accept(lex::FOR)) {
			if(p.peekt(1) == lex::IDEN && p.peekt(2) == lex::FIN) {
				if(!parseForIn(stmt)) return false;
			} else {
				if(!parseFor(stmt)) return false;
			}
			skip_cols = true;
		} else if(p.accept(lex::WHILE)) {
			if(!parseWhile(stmt)) return false;
			skip_cols = true;
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
			skip_cols = true;
		} else if(p.accept(lex::LBRACE)) {
			if(!parseBlock((StmtBlock *&)stmt)) return false;
			skip_cols = true;
		} else if(!parseExpr(stmt)) {
			return false;
		}

		if(skip_cols || p.acceptn(lex::COLS)) {
			stmts.push_back(stmt);
			stmt = nullptr;
			continue;
		}
		err.fail(p.peek().getLoc(), "expected semicolon for end of statement, found: ",
			 p.peek().getTok().cStr());
		return false;
	}

	if(with_brace) {
		if(!p.acceptn(lex::RBRACE)) {
			err.fail(
			p.peek().getLoc(),
			"expected closing braces '}' for block, found: ", p.peek().getTok().cStr());
			return false;
		}
	}

	tree = StmtBlock::create(allocator, start.getLoc(), stmts, !with_brace);
	return true;
}

bool Parser::parseSimple(Stmt *&data)
{
	data = nullptr;

	if(!p.peek().getTok().isData()) {
		err.fail(p.peek().getLoc(),
			 "expected data here, found: ", p.peek().getTok().cStr());
		return false;
	}

	lex::Lexeme &val = p.peek();
	p.next();

	data = StmtSimple::create(allocator, val.getLoc(), val);
	return true;
}

// ref"Ref of this"
// 9h
// 2.5i
bool Parser::parsePrefixedSuffixedLiteral(Stmt *&expr)
{
	lex::Lexeme &iden = p.peekt() == lex::IDEN ? p.peek() : p.peek(1);
	lex::Lexeme &lit  = p.peekt() == lex::IDEN ? p.peek(1) : p.peek();
	lex::Lexeme oper  = lex::Lexeme(iden.getLoc(), lex::TokType::FNCALL);

	p.next();
	p.next();

	StmtSimple *arg	  = StmtSimple::create(allocator, lit.getLoc(), lit);
	StmtSimple *fn	  = StmtSimple::create(allocator, iden.getLoc(), iden);
	StmtFnArgs *finfo = StmtFnArgs::create(allocator, arg->getLoc(), {arg}, {false});
	expr		  = StmtExpr::create(allocator, lit.getLoc(), fn, oper, finfo);

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

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	size_t commas = 0;

	if(!parseExpr17(rhs)) {
		return false;
	}

	while(p.accept(lex::COMMA)) {
		++commas;
		oper = p.peek();
		p.next();
		if(!parseExpr17(lhs)) {
			return false;
		}
		rhs = StmtExpr::create(allocator, start.getLoc(), lhs, oper, rhs);
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

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr15(rhs)) {
		return false;
	}

	while(p.accept(lex::ASSN)) {
		oper = p.peek();
		p.next();
		if(!parseExpr15(lhs)) {
			return false;
		}
		rhs = StmtExpr::create(allocator, start.getLoc(), lhs, oper, rhs);
		lhs = nullptr;
	}

	expr = rhs;
	return true;
}
// Left Associative
// += -= *=
// /= %= <<=
// >>= &= |=
// ~= ^= ??=
// or-block
bool Parser::parseExpr16(Stmt *&expr)
{
	expr = nullptr;

	Stmt *lhs	  = nullptr;
	Stmt *rhs	  = nullptr;
	StmtBlock *or_blk = nullptr;
	lex::Lexeme or_blk_var;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr14(lhs)) {
		return false;
	}

	while(p.accept(lex::ADD_ASSN, lex::SUB_ASSN, lex::MUL_ASSN) ||
	      p.accept(lex::DIV_ASSN, lex::MOD_ASSN, lex::LSHIFT_ASSN) ||
	      p.accept(lex::RSHIFT_ASSN, lex::BAND_ASSN, lex::BOR_ASSN) ||
	      p.accept(lex::BNOT_ASSN, lex::BXOR_ASSN, lex::NIL_COALESCE_ASSN))
	{
		oper = p.peek();
		p.next();
		if(!parseExpr14(rhs)) {
			return false;
		}
		lhs = StmtExpr::create(allocator, start.getLoc(), lhs, oper, rhs);
		rhs = nullptr;
	}

	expr = lhs;

	if(!p.acceptn(lex::OR)) return true;

	if(p.accept(lex::IDEN)) {
		or_blk_var = p.peek();
		p.next();
	}

	if(!parseBlock(or_blk)) {
		return false;
	}
	if(expr->getStmtType() != EXPR) {
		expr = StmtExpr::create(allocator, expr->getLoc(), expr, {}, nullptr);
	}
	as<StmtExpr>(expr)->setOr(or_blk, or_blk_var);
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

	lex::Lexeme &start = p.peek();

	if(!parseExpr16(cond)) {
		return false;
	}
	if(!p.acceptn(lex::QUEST)) {
		expr = cond;
		return true;
	}

	if(!parseExpr16(blkStmt)) {
		return false;
	}
	if(!p.acceptn(lex::COL)) {
		err.fail(p.peek().getLoc(),
			 "expected ':' for ternary operator, found: ", p.peek().getTok().cStr());
		return false;
	}
	blkStmt = StmtBlock::create(allocator, blkStmt->getLoc(), {blkStmt}, true);
	as<StmtBlock>(blkStmt)->setUnload(false);
	cvec.emplace_back(cond, as<StmtBlock>(blkStmt));
	blkStmt = nullptr;
	if(!parseExpr16(blkStmt)) {
		return false;
	}
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

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr13(lhs)) {
		return false;
	}

	while(p.accept(lex::NIL_COALESCE)) {
		oper = p.peek();
		p.next();
		if(!parseExpr13(rhs)) {
			return false;
		}
		lhs = StmtExpr::create(allocator, start.getLoc(), lhs, oper, rhs);
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

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr12(lhs)) {
		return false;
	}

	while(p.accept(lex::LOR)) {
		oper = p.peek();
		p.next();
		if(!parseExpr12(rhs)) {
			return false;
		}
		lhs = StmtExpr::create(allocator, start.getLoc(), lhs, oper, rhs);
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

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr11(lhs)) {
		return false;
	}

	while(p.accept(lex::LAND)) {
		oper = p.peek();
		p.next();
		if(!parseExpr11(rhs)) {
			return false;
		}
		lhs = StmtExpr::create(allocator, start.getLoc(), lhs, oper, rhs);
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

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr10(lhs)) {
		return false;
	}

	while(p.accept(lex::BOR)) {
		oper = p.peek();
		p.next();
		if(!parseExpr10(rhs)) {
			return false;
		}
		lhs = StmtExpr::create(allocator, start.getLoc(), lhs, oper, rhs);
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

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr09(lhs)) {
		return false;
	}

	while(p.accept(lex::BXOR)) {
		oper = p.peek();
		p.next();
		if(!parseExpr09(rhs)) {
			return false;
		}
		lhs = StmtExpr::create(allocator, start.getLoc(), lhs, oper, rhs);
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

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr08(lhs)) {
		return false;
	}

	while(p.accept(lex::BAND)) {
		oper = p.peek();
		p.next();
		if(!parseExpr08(rhs)) {
			return false;
		}
		lhs = StmtExpr::create(allocator, start.getLoc(), lhs, oper, rhs);
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

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr07(lhs)) {
		return false;
	}

	while(p.accept(lex::EQ, lex::NE)) {
		oper = p.peek();
		p.next();
		if(!parseExpr07(rhs)) {
			return false;
		}
		lhs = StmtExpr::create(allocator, start.getLoc(), lhs, oper, rhs);
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

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr06(lhs)) {
		return false;
	}

	while(p.accept(lex::LT, lex::LE) || p.accept(lex::GT, lex::GE)) {
		oper = p.peek();
		p.next();
		if(!parseExpr06(rhs)) {
			return false;
		}
		lhs = StmtExpr::create(allocator, start.getLoc(), lhs, oper, rhs);
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

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr05(lhs)) {
		return false;
	}

	while(p.accept(lex::LSHIFT, lex::RSHIFT)) {
		oper = p.peek();
		p.next();
		if(!parseExpr05(rhs)) {
			return false;
		}
		lhs = StmtExpr::create(allocator, start.getLoc(), lhs, oper, rhs);
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

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr04(lhs)) {
		return false;
	}

	while(p.accept(lex::ADD, lex::SUB)) {
		oper = p.peek();
		p.next();
		if(!parseExpr04(rhs)) {
			return false;
		}
		lhs = StmtExpr::create(allocator, start.getLoc(), lhs, oper, rhs);
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

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr03(lhs)) {
		return false;
	}

	while(p.accept(lex::MUL, lex::DIV, lex::MOD) || p.accept(lex::POWER, lex::ROOT)) {
		oper = p.peek();
		p.next();
		if(!parseExpr03(rhs)) {
			return false;
		}
		lhs = StmtExpr::create(allocator, start.getLoc(), lhs, oper, rhs);
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

	Vector<lex::Lexeme> opers;

	lex::Lexeme &start = p.peek();

	while(p.accept(lex::XINC, lex::XDEC) || p.accept(lex::ADD, lex::SUB) ||
	      p.accept(lex::MUL, lex::BAND) || p.accept(lex::LNOT, lex::BNOT))
	{
		if(p.peekt() == lex::XINC) p.sett(lex::INCX);
		if(p.peekt() == lex::XDEC) p.sett(lex::DECX);
		if(p.peekt() == lex::ADD) p.sett(lex::UADD);
		if(p.peekt() == lex::SUB) p.sett(lex::USUB);
		if(p.peekt() == lex::MUL) p.sett(lex::UMUL);
		if(p.peekt() == lex::BAND) p.sett(lex::UAND);
		opers.insert(opers.begin(), p.peek());
		p.next();
	}

	if(!parseExpr02(lhs)) {
		return false;
	}

	if(!lhs) {
		err.fail(start.getLoc(), "invalid expression");
		return false;
	}

	if(lhs->isSimple() && !opers.empty()) {
		lex::Lexeme &val = as<StmtSimple>(lhs)->getLexValue();
		lex::TokType tk	 = val.getTokVal();
		if(tk == lex::INT) {
			while(!opers.empty() && opers.front().getTokVal() == lex::USUB) {
				val.setDataInt(-val.getDataInt());
				opers.erase(opers.begin());
			}
		}
		if(tk == lex::FLT) {
			while(!opers.empty() && opers.front().getTokVal() == lex::USUB) {
				val.setDataFlt(-val.getDataFlt());
				opers.erase(opers.begin());
			}
		}
	}

	for(auto &op : opers) {
		lhs = StmtExpr::create(allocator, op.getLoc(), lhs, op, nullptr);
	}

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

	Vector<lex::Lexeme> opers;

	lex::Lexeme &start = p.peek();

	if(!parseExpr01(lhs)) {
		return false;
	}

	if(p.accept(lex::XINC, lex::XDEC, lex::PreVA)) {
		if(p.accept(lex::PreVA)) p.sett(lex::PostVA);
		lhs = StmtExpr::create(allocator, p.peek().getLoc(), lhs, p.peek(), nullptr);
		p.next();
	}

	expr = lhs;
	return true;
}
bool Parser::parseExpr01(Stmt *&expr)
{
	expr = nullptr;

	lex::Lexeme &start = p.peek();
	lex::Lexeme dot;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;
	Vector<Stmt *> args;
	Stmt *arg = nullptr;
	Vector<bool> unpack_vector; // works for variadic as well

	// prefixed/suffixed literals
	if(p.accept(lex::IDEN) && p.peek(1).getTok().isLiteral() ||
	   p.peek().getTok().isLiteral() && p.peekt(1) == lex::IDEN)
	{
		return parsePrefixedSuffixedLiteral(expr);
	}

	if(p.acceptn(lex::LPAREN)) {
		if(!parseExpr(lhs)) {
			return false;
		}
		if(!p.acceptn(lex::RPAREN)) {
			err.fail(p.peek().getLoc(),
				 "expected ending parenthesis ')' for expression, found: ",
				 p.peek().getTok().cStr());
			return false;
		}
	}

	if(p.acceptd() && !parseSimple(lhs)) {
		err.fail(p.peek().getLoc(), "failed to parse simple");
		return false;
	}
	if(p.accept(lex::FN) && !parseFnDef(lhs)) return false;
	goto begin_brack;

after_dot:
	if(!p.acceptd() || !parseSimple(rhs)) return false;
	if(lhs && rhs) {
		lhs = StmtExpr::create(allocator, dot.getLoc(), lhs, dot, rhs);
		rhs = nullptr;
	}

begin_brack:
	if(p.accept(lex::LBRACK)) {
		lex::Lexeme oper;
		p.sett(lex::SUBS);
		oper = p.peek();
		p.next();
		if(!parseExpr17(rhs)) {
			err.fail(oper.getLoc(), "failed to parse expression for subscript");
			return false;
		}
		if(!p.acceptn(lex::RBRACK)) {
			err.fail(p.peek().getLoc(),
				 "expected closing bracket for"
				 " subscript expression, found: ",
				 p.peek().getTok().cStr());
			return false;
		}
		lhs = StmtExpr::create(allocator, oper.getLoc(), lhs, oper, rhs);
		rhs = nullptr;
		if(p.accept(lex::LBRACK, lex::LPAREN) ||
		   (p.peekt() == lex::DOT && p.peekt(1) == lex::LT))
			goto begin_brack;
	} else if(p.accept(lex::LPAREN)) {
		if(!p.accept(lex::LPAREN)) {
			err.fail(p.peek().getLoc(),
				 "expected opening parenthesis"
				 " for a call, found: ",
				 p.peek().getTok().cStr());
			return false;
		}
		lex::Lexeme oper = p.peek();
		oper.getTok().setVal(lex::FNCALL);
		p.next();
		if(p.acceptn(lex::RPAREN)) goto post_args;
		// parse arguments
		while(true) {
			if(p.accept(lex::STR, lex::IDEN) && p.peekt(1) == lex::ASSN) {
				// assn args (begins with <STR/IDEN> '=')
				if(p.accept(lex::IDEN)) p.sett(lex::STR);
				lex::Lexeme &name = p.peek();
				p.next();
				p.next();
				if(!parseExpr17(arg)) return false;
				arg =
				StmtVar::create(allocator, name.getLoc(), name, nullptr, arg, true);
			} else if(!parseExpr17(arg)) { // normal arg
				return false;
			}
			unpack_vector.push_back(
			arg->isExpr() && as<StmtExpr>(arg)->getOperTok().isType(lex::PostVA));
			if(unpack_vector.back()) arg = as<StmtExpr>(arg)->getLHS();
			args.push_back(arg);
			arg = nullptr;
			if(!p.acceptn(lex::COMMA)) break;
		}
		if(!p.acceptn(lex::RPAREN)) {
			err.fail(p.peek().getLoc(),
				 "expected closing parenthesis/brace after "
				 "a call arguments, found: ",
				 p.peek().getTok().cStr());
			return false;
		}
	post_args:
		rhs	      = StmtFnArgs::create(allocator, oper.getLoc(), std::move(args),
						   std::move(unpack_vector));
		lhs	      = StmtExpr::create(allocator, oper.getLoc(), lhs, oper, rhs);
		rhs	      = nullptr;
		args	      = {};
		unpack_vector = {};

		if(p.accept(lex::LBRACK, lex::LPAREN)) goto begin_brack;
	}

	if(p.acceptn(lex::DOT, lex::ARROW)) {
		if(lhs && rhs) {
			lhs =
			StmtExpr::create(allocator, p.peek(-1).getLoc(), lhs, p.peek(-1), rhs);
			rhs = nullptr;
		}
		dot = p.peek(-1);
		goto after_dot;
	}

	if(lhs && rhs) {
		lhs = StmtExpr::create(allocator, dot.getLoc(), lhs, dot, rhs);
		rhs = nullptr;
	}
	expr = lhs;
	return true;
}

bool Parser::parseVar(StmtVar *&var, bool is_fn_arg)
{
	var = nullptr;

	if(!p.accept(lex::IDEN, lex::STR)) {
		err.fail(p.peek().getLoc(), "expected identifier for variable name, found: ",
			 p.peek().getTok().cStr());
		return false;
	}
	lex::Lexeme &name = p.peek();
	p.next();
	Stmt *val = nullptr;
	Stmt *in  = nullptr;

	if(p.acceptn(lex::FIN) && !is_fn_arg) {
		if(!parseExpr01((Stmt *&)in)) {
			err.fail(p.peek().getLoc(),
				 "failed to parse in-type for variable: ", name.getDataStr());
			return false;
		}
	}

	if(!p.acceptn(lex::ASSN)) {
		if(is_fn_arg) goto end;
		err.fail(name.getLoc(), "invalid variable declaration - no value set");
		return false;
	}
	if(!parseExpr17(val)) return false;

end:
	var = StmtVar::create(allocator, name.getLoc(), name, in, val, is_fn_arg);
	return true;
}

bool Parser::parseFnSig(Stmt *&fsig)
{
	fsig = nullptr;

	Vector<StmtVar *> args;
	StmtVar *arg = nullptr;
	Set<StringRef> argnames;
	StmtSimple *kwarg = nullptr, *vaarg = nullptr;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::FN)) {
		err.fail(p.peek().getLoc(),
			 "expected 'fn' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(!p.acceptn(lex::LPAREN)) {
		err.fail(p.peek().getLoc(),
			 "expected opening parenthesis for function args, found: ",
			 p.peek().getTok().cStr());
		return false;
	}
	if(p.acceptn(lex::RPAREN)) goto post_args;

	// args
	while(true) {
		bool attempt_kw = false;
		if(p.accept(lex::STR)) attempt_kw = true;
		if(!p.accept(lex::IDEN, lex::STR)) {
			err.fail(p.peek().getLoc(), "expected identifier/str for argument, found: ",
				 p.peek().getTok().cStr());
			return false;
		}
		if(argnames.find(p.peek().getDataStr()) != argnames.end()) {
			err.fail(p.peek().getLoc(), "this argument name is already used "
						    "before in this function signature");
			return false;
		}
		argnames.insert(p.peek().getDataStr());
		// this is a keyword arg
		if(attempt_kw) {
			if(kwarg) {
				err.fail(p.peek().getLoc(),
					 "function cannot have multiple"
					 " keyword arguments (previous: ",
					 kwarg->getLexDataStr(), ")");
				return false;
			}
			kwarg = StmtSimple::create(allocator, p.peek().getLoc(), p.peek());
			p.next();
		} else if(p.peekt(1) == lex::PreVA) {
			p.peek(1).getTok().setVal(lex::PostVA);
			// no check for multiple variadic as no arg can exist after a variadic
			vaarg = StmtSimple::create(allocator, p.peek().getLoc(), p.peek());
			p.next();
			p.next();
		} else {
			if(!parseVar(arg, true)) {
				err.fail(p.peek().getLoc(),
					 "failed to parse function definition parameter");
				return false;
			}
			args.push_back(arg);
			arg = nullptr;
		}
		if(!p.acceptn(lex::COMMA)) break;
		if(vaarg) {
			err.fail(p.peek().getLoc(), "no parameter can exist after variadic");
			return false;
		}
	}

	if(!p.acceptn(lex::RPAREN)) {
		err.fail(p.peek().getLoc(),
			 "expected closing parenthesis after function args, found: ",
			 p.peek().getTok().cStr());
		return false;
	}

post_args:
	fsig = StmtFnSig::create(allocator, start.getLoc(), args, kwarg, vaarg);
	return true;
}
bool Parser::parseFnDef(Stmt *&fndef)
{
	fndef = nullptr;

	Stmt *sig	   = nullptr;
	StmtBlock *blk	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(!parseFnSig(sig)) return false;
	if(!parseBlock(blk)) return false;

	// append a return statement if the block doesn't already contain one at the end
	if(blk) {
		auto &stmts = blk->getStmts();
		if(!stmts.empty() && !stmts.back()->isReturn()) {
			stmts.emplace_back(StmtRet::create(allocator, blk->getLoc(), nullptr));
		}
	}

	fndef = StmtFnDef::create(allocator, start.getLoc(), (StmtFnSig *)sig, blk);
	return true;
}

bool Parser::parseVardecl(Stmt *&vd)
{
	vd = nullptr;

	Vector<StmtVar *> decls;
	StmtVar *decl	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::LET)) {
		err.fail(p.peek().getLoc(),
			 "expected 'let' keyword here, found: ", p.peek().getTok().cStr());
		return false;
	}

	while(p.accept(lex::IDEN, lex::STR)) {
		if(!parseVar(decl, false)) return false;
		decls.push_back(decl);
		decl = nullptr;
		if(!p.acceptn(lex::COMMA)) break;
	}

	vd = StmtVarDecl::create(allocator, start.getLoc(), decls);
	return true;
}

bool Parser::parseConds(Stmt *&conds)
{
	conds = nullptr;

	Vector<Conditional> cvec;
	Conditional c(nullptr, nullptr);

	lex::Lexeme &start = p.peek();

	bool is_inline = p.acceptn(lex::INLINE);

cond:
	if(!p.acceptn(lex::IF, lex::ELIF)) {
		err.fail(p.peek().getLoc(),
			 "expected 'if' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(!parseExpr16(c.getCond())) {
		err.fail(p.peek().getLoc(), "failed to parse condition for if/else if statement");
		return false;
	}

blk:
	if(!parseBlock(c.getBlk())) {
		err.fail(p.peek().getLoc(), "failed to parse block for conditional");
		return false;
	}
	// If the conditional is inline, the block shouldn't generate PUSH/POP_BLK instructions.
	if(is_inline) c.getBlk()->setTop(is_inline);

	cvec.emplace_back(c.getCond(), c.getBlk());
	c.reset();

	if(p.accept(lex::ELIF)) goto cond;
	if(p.acceptn(lex::ELSE)) goto blk;

	conds = StmtCond::create(allocator, start.getLoc(), cvec);
	return true;
}
// For-In transformation:
//
// for e in vec.eachRev() {
// 	...
// }
// ----------------------
// will generate
// ----------------------
// LOOP_BEGIN
// let __e = vec.eachRev();
// INIT:
// let x = __e.next();
// {
// 	if x is nil, jump to LOOP_END (done using JMP_NIL instr)
// 	...
// }
// JMP INIT
// LOOP_END
bool Parser::parseForIn(Stmt *&fin)
{
	fin = nullptr;

	Stmt *in	   = nullptr; // Expr15
	StmtBlock *blk	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::FOR)) {
		err.fail(p.peek().getLoc(),
			 "expected 'for' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(!p.accept(lex::IDEN)) {
		err.fail(p.peek().getLoc(),
			 "expected iterator (identifier) here, found: ", p.peek().getTok().cStr());
		return false;
	}
	lex::Lexeme &iter = p.peek();
	p.next();

	if(!p.acceptn(lex::FIN)) {
		err.fail(p.peek().getLoc(),
			 "expected 'in' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(!parseExpr16(in)) {
		err.fail(p.peek().getLoc(), "failed to parse expression for 'in'");
		return false;
	}

	if(!p.accept(lex::LBRACE)) {
		err.fail(p.peek().getLoc(),
			 "expected block for for-in construct, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(!parseBlock(blk)) {
		err.fail(p.peek().getLoc(), "failed to parse block for for-in construct");
		return false;
	}

	fin = StmtForIn::create(allocator, start.getLoc(), iter, in, blk);
	return true;
}
bool Parser::parseFor(Stmt *&f)
{
	f = nullptr;

	Stmt *init	   = nullptr; // either of StmtVarDecl or StmtExpr
	Stmt *cond	   = nullptr;
	Stmt *incr	   = nullptr;
	StmtBlock *blk	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::FOR)) {
		err.fail(p.peek().getLoc(),
			 "expected 'for' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(p.acceptn(lex::COLS)) goto cond;

	if(p.accept(lex::LET)) {
		if(!parseVardecl(init)) return false;
	} else {
		if(!parseExpr(init)) return false;
	}
	if(!p.acceptn(lex::COLS)) {
		err.fail(p.peek().getLoc(),
			 "expected semicolon here, found: ", p.peek().getTok().cStr());
		return false;
	}

cond:
	if(p.acceptn(lex::COLS)) goto incr;

	if(!parseExpr17(cond)) return false;
	if(!p.acceptn(lex::COLS)) {
		err.fail(p.peek().getLoc(),
			 "expected semicolon here, found: ", p.peek().getTok().cStr());
		return false;
	}

incr:
	if(p.accept(lex::LBRACE)) goto body;

	if(!parseExpr(incr)) return false;
	if(!p.accept(lex::LBRACE)) {
		err.fail(p.peek().getLoc(),
			 "expected braces for body here, found: ", p.peek().getTok().cStr());
		return false;
	}

body:
	if(!parseBlock(blk)) {
		err.fail(p.peek().getLoc(), "failed to parse block for 'for' construct");
		return false;
	}

	f = StmtFor::create(allocator, start.getLoc(), init, cond, incr, blk);
	return true;
}
bool Parser::parseWhile(Stmt *&w)
{
	w = nullptr;

	Stmt *cond	   = nullptr;
	StmtBlock *blk	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::WHILE)) {
		err.fail(p.peek().getLoc(),
			 "expected 'while' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(!parseExpr17(cond)) return false;

	if(!parseBlock(blk)) {
		err.fail(p.peek().getLoc(), "failed to parse block for 'for' construct");
		return false;
	}

	w = StmtFor::create(allocator, start.getLoc(), nullptr, cond, nullptr, blk);
	return true;
}
bool Parser::parseRet(Stmt *&ret)
{
	ret = nullptr;

	Stmt *val	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::RETURN)) {
		err.fail(p.peek().getLoc(),
			 "expected 'return' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(p.accept(lex::COLS)) goto done;

	if(!parseExpr17(val)) {
		lex::Lexeme &l = p.isValid() ? p.peek() : start;
		err.fail(l.getLoc(), "failed to parse expression for return value");
		return false;
	}

done:
	ret = StmtRet::create(allocator, start.getLoc(), val);
	return true;
}
bool Parser::parseContinue(Stmt *&cont)
{
	cont = nullptr;

	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::CONTINUE)) {
		err.fail(p.peek().getLoc(),
			 "expected 'continue' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	cont = StmtContinue::create(allocator, start.getLoc());
	return true;
}
bool Parser::parseBreak(Stmt *&brk)
{
	brk = nullptr;

	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::BREAK)) {
		err.fail(p.peek().getLoc(),
			 "expected 'break' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	brk = StmtBreak::create(allocator, start.getLoc());
	return true;
}
bool Parser::parseDefer(Stmt *&defer)
{
	defer = nullptr;

	Stmt *val	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::DEFER)) {
		err.fail(p.peek().getLoc(),
			 "expected 'defer' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(!parseExpr17(val)) {
		err.fail(p.peek().getLoc(), "failed to parse expression for defer value");
		return false;
	}

done:
	defer = StmtDefer::create(allocator, start.getLoc(), val);
	return true;
}

} // namespace fer::ast
