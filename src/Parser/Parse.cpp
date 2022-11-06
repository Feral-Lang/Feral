#include "Parser/Parse.hpp"

#include <unordered_set>

#include "Error.hpp"

namespace fer
{

Parser::Parser(Context &ctx) : ctx(ctx) {}

// on successful parse, returns true, and tree is allocated
// if with_brace is true, it will attempt to find the beginning and ending brace for each block
bool Parser::parseBlock(ParseHelper &p, StmtBlock *&tree, bool with_brace)
{
	tree = nullptr;

	Vector<Stmt *> stmts;
	Stmt *stmt = nullptr;

	lex::Lexeme &start = p.peek();

	if(with_brace) {
		if(!p.acceptn(lex::LBRACE)) {
			err::out(p.peek(), {"expected opening braces '{' for block, found: ",
					    p.peek().getTok().cStr()});
			return false;
		}
	}

	while(p.isValid() && (!with_brace || !p.accept(lex::RBRACE))) {
		bool skip_cols = false;
		// logic
		if(p.accept(lex::LET)) {
			if(!parseVardecl(p, stmt)) return false;
		} else if(p.accept(lex::IF)) {
			if(!parseConds(p, stmt)) return false;
			skip_cols = true;
		} else if(p.accept(lex::FOR)) {
			if(p.peekt(1) == lex::IDEN && p.peekt(2) == lex::IN) {
				if(!parseForIn(p, stmt)) return false;
			} else {
				if(!parseFor(p, stmt)) return false;
			}
			skip_cols = true;
		} else if(p.accept(lex::WHILE)) {
			if(!parseWhile(p, stmt)) return false;
			skip_cols = true;
		} else if(p.accept(lex::RETURN)) {
			if(!parseRet(p, stmt)) return false;
		} else if(p.accept(lex::CONTINUE)) {
			if(!parseContinue(p, stmt)) return false;
		} else if(p.accept(lex::BREAK)) {
			if(!parseBreak(p, stmt)) return false;
		} else if(p.accept(lex::DEFER)) {
			if(!parseDefer(p, stmt)) return false;
		} else if(p.accept(lex::LBRACE)) {
			if(!parseBlock(p, (StmtBlock *&)stmt)) return false;
			skip_cols = true;
		} else if(!parseExpr(p, stmt, false)) {
			return false;
		}

		if(skip_cols || p.acceptn(lex::COLS)) {
			stmts.push_back(stmt);
			stmt = nullptr;
			continue;
		}
		err::out(p.peek(), {"expected semicolon for end of statement, found: ",
				    p.peek().getTok().cStr()});
		return false;
	}

	if(with_brace) {
		if(!p.acceptn(lex::RBRACE)) {
			err::out(p.peek(), {"expected closing braces '}' for block, found: ",
					    p.peek().getTok().cStr()});
			return false;
		}
	}

	tree = StmtBlock::create(ctx, start.getLoc(), stmts, !with_brace);
	return true;
}

bool Parser::parseSimple(ParseHelper &p, Stmt *&data)
{
	data = nullptr;

	if(!p.peek().getTok().isData()) {
		err::out(p.peek(), {"expected data here, found: ", p.peek().getTok().cStr()});
		return false;
	}

	lex::Lexeme &val = p.peek();
	p.next();

	data = StmtSimple::create(ctx, val.getLoc(), val);
	return true;
}

// ref"Ref of this"
// 9h
// 2.5i
bool Parser::parsePrefixedSuffixedLiteral(ParseHelper &p, Stmt *&expr)
{
	lex::Lexeme &iden = p.peekt() == lex::IDEN ? p.peek() : p.peek(1);
	lex::Lexeme &lit  = p.peekt() == lex::IDEN ? p.peek(1) : p.peek();
	lex::Lexeme oper  = lex::Lexeme(iden.getLoc(), lex::TokType::FNCALL);

	p.next();
	p.next();

	StmtSimple *arg	  = StmtSimple::create(ctx, lit.getLoc(), lit);
	StmtSimple *fn	  = StmtSimple::create(ctx, iden.getLoc(), iden);
	StmtFnArgs *finfo = StmtFnArgs::create(ctx, arg->getLoc(), {arg});
	expr		  = StmtExpr::create(ctx, lit.getLoc(), fn, oper, finfo);

	return true;
}

bool Parser::parseExpr(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	return parseExpr17(p, expr, disable_brace_after_iden);
}

// Left Associative
// ,
bool Parser::parseExpr17(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	size_t commas = 0;

	if(!parseExpr16(p, rhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::COMMA)) {
		++commas;
		oper = p.peek();
		p.next();
		if(!parseExpr16(p, lhs, disable_brace_after_iden)) {
			return false;
		}
		rhs = StmtExpr::create(ctx, start.getLoc(), lhs, oper, rhs);
		lhs = nullptr;
	}

	expr = rhs;
	return true;
}
// Left Associative
// ?:
bool Parser::parseExpr16(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs     = nullptr;
	Stmt *rhs     = nullptr;
	Stmt *lhs_lhs = nullptr;
	Stmt *lhs_rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr15(p, lhs, disable_brace_after_iden)) {
		return false;
	}
	if(!p.accept(lex::QUEST)) {
		expr = lhs;
		return true;
	}

	oper = p.peek();
	p.next();
	lex::Lexeme oper_inside;

	if(!parseExpr15(p, lhs_lhs, disable_brace_after_iden)) {
		return false;
	}
	if(!p.accept(lex::COL)) {
		err::out(p.peek(),
			 {"expected ':' for ternary operator, found: ", p.peek().getTok().cStr()});
		return false;
	}
	oper_inside = p.peek();
	p.next();
	if(!parseExpr15(p, lhs_rhs, disable_brace_after_iden)) {
		return false;
	}
	rhs = StmtExpr::create(ctx, oper.getLoc(), lhs_lhs, oper_inside, lhs_rhs);
	goto after_quest;

after_quest:
	expr = StmtExpr::create(ctx, start.getLoc(), lhs, oper, rhs);
	return true;
}
// Right Associative
// =
bool Parser::parseExpr15(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr14(p, rhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::ASSN)) {
		oper = p.peek();
		p.next();
		if(!parseExpr14(p, lhs, disable_brace_after_iden)) {
			return false;
		}
		rhs = StmtExpr::create(ctx, start.getLoc(), lhs, oper, rhs);
		lhs = nullptr;
	}

	expr = rhs;
	return true;
}
// Left Associative
// += -=
// *= /= %=
// <<= >>=
// &= |= ^=
// or-block
bool Parser::parseExpr14(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs	  = nullptr;
	Stmt *rhs	  = nullptr;
	StmtBlock *or_blk = nullptr;
	lex::Lexeme or_blk_var;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr13(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::ADD_ASSN, lex::SUB_ASSN, lex::MUL_ASSN) ||
	      p.accept(lex::DIV_ASSN, lex::MOD_ASSN, lex::LSHIFT_ASSN) ||
	      p.accept(lex::RSHIFT_ASSN, lex::BAND_ASSN, lex::BOR_ASSN) ||
	      p.accept(lex::BNOT_ASSN, lex::BXOR_ASSN))
	{
		oper = p.peek();
		p.next();
		if(!parseExpr13(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), lhs, oper, rhs);
		rhs = nullptr;
	}

	expr = lhs;

	if(!p.acceptn(lex::OR)) return true;

	if(p.accept(lex::IDEN)) {
		or_blk_var = p.peek();
		p.next();
	}

	if(!parseBlock(p, or_blk)) {
		return false;
	}
	if(expr->getStmtType() != EXPR) {
		expr = StmtExpr::create(ctx, expr->getLoc(), expr, {}, nullptr);
	}
	as<StmtExpr>(expr)->setOr(or_blk, or_blk_var);
	return true;
}
// Left Associative
// ||
bool Parser::parseExpr13(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr12(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::LOR)) {
		oper = p.peek();
		p.next();
		if(!parseExpr12(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), lhs, oper, rhs);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Left Associative
// &&
bool Parser::parseExpr12(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr11(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::LAND)) {
		oper = p.peek();
		p.next();
		if(!parseExpr11(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), lhs, oper, rhs);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Left Associative
// |
bool Parser::parseExpr11(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr10(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::BOR)) {
		oper = p.peek();
		p.next();
		if(!parseExpr10(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), lhs, oper, rhs);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Left Associative
// ^
bool Parser::parseExpr10(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr09(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::BXOR)) {
		oper = p.peek();
		p.next();
		if(!parseExpr09(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), lhs, oper, rhs);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Left Associative
// &
bool Parser::parseExpr09(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr08(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::BAND)) {
		oper = p.peek();
		p.next();
		if(!parseExpr08(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), lhs, oper, rhs);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Left Associative
// == !=
bool Parser::parseExpr08(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr07(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::EQ, lex::NE)) {
		oper = p.peek();
		p.next();
		if(!parseExpr07(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), lhs, oper, rhs);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Left Associative
// < <=
// > >=
bool Parser::parseExpr07(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr06(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::LT, lex::LE) || p.accept(lex::GT, lex::GE)) {
		oper = p.peek();
		p.next();
		if(!parseExpr06(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), lhs, oper, rhs);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Left Associative
// << >>
bool Parser::parseExpr06(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr05(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::LSHIFT, lex::RSHIFT)) {
		oper = p.peek();
		p.next();
		if(!parseExpr05(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), lhs, oper, rhs);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Left Associative
// + -
bool Parser::parseExpr05(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr04(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::ADD, lex::SUB)) {
		oper = p.peek();
		p.next();
		if(!parseExpr04(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), lhs, oper, rhs);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Left Associative
// * / %
bool Parser::parseExpr04(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr03(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::MUL, lex::DIV, lex::MOD)) {
		oper = p.peek();
		p.next();
		if(!parseExpr03(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), lhs, oper, rhs);
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
bool Parser::parseExpr03(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
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

	if(!parseExpr02(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	if(!lhs) {
		err::out(start, {"invalid expression"});
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
		lhs = StmtExpr::create(ctx, op.getLoc(), lhs, op, nullptr);
	}

	expr = lhs;
	return true;
}
// Left Associative
// ++ -- (post)
// ... (postva)
bool Parser::parseExpr02(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;

	Vector<lex::Lexeme> opers;

	lex::Lexeme &start = p.peek();

	if(!parseExpr01(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	if(p.accept(lex::XINC, lex::XDEC, lex::PreVA)) {
		if(p.peekt() == lex::PreVA) p.sett(lex::PostVA);
		lhs = StmtExpr::create(ctx, p.peek().getLoc(), lhs, p.peek(), nullptr);
		p.next();
	}

	expr = lhs;
	return true;
}
bool Parser::parseExpr01(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	lex::Lexeme &start = p.peek();
	lex::Lexeme dot;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;
	Vector<Stmt *> args;
	Stmt *arg	  = nullptr;
	bool is_intrinsic = false;

	// prefixed/suffixed literals
	if(p.accept(lex::IDEN) && p.peek(1).getTok().isLiteral() ||
	   p.peek().getTok().isLiteral() && p.peekt(1) == lex::IDEN)
	{
		return parsePrefixedSuffixedLiteral(p, expr);
	}

	if(p.acceptn(lex::LPAREN)) {
		if(!parseExpr(p, lhs, disable_brace_after_iden)) {
			return false;
		}
		if(!p.acceptn(lex::RPAREN)) {
			err::out(p.peek(),
				 {"expected ending parenthesis ')' for expression, found: ",
				  p.peek().getTok().cStr()});
			return false;
		}
	}

begin:
	if(p.acceptn(lex::AT)) is_intrinsic = true;
	if(p.acceptd() && !parseSimple(p, lhs)) return false;
	goto begin_brack;

after_dot:
	if(!p.acceptd() || !parseSimple(p, rhs)) return false;
	if(lhs && rhs) {
		lhs = StmtExpr::create(ctx, dot.getLoc(), lhs, dot, rhs);
		rhs = nullptr;
	}

begin_brack:
	if(p.accept(lex::LBRACK)) {
		lex::Lexeme oper;
		p.sett(lex::SUBS);
		oper = p.peek();
		p.next();
		if(is_intrinsic) {
			err::out(p.peek(), {"only function calls can be intrinsic;"
					    " attempted subscript here"});
			return false;
		}
		if(!parseExpr16(p, rhs, false)) {
			err::out(oper, {"failed to parse expression for subscript"});
			return false;
		}
		if(!p.acceptn(lex::RBRACK)) {
			err::out(p.peek(), {"expected closing bracket for"
					    " subscript expression, found: ",
					    p.peek().getTok().cStr()});
			return false;
		}
		lhs = StmtExpr::create(ctx, oper.getLoc(), lhs, oper, rhs);
		rhs = nullptr;
		if(p.accept(lex::LBRACK, lex::LPAREN) ||
		   (p.peekt() == lex::DOT && p.peekt(1) == lex::LT))
			goto begin_brack;
	} else if(p.accept(lex::LPAREN) || (!disable_brace_after_iden && p.accept(lex::LBRACE))) {
		bool fncall = p.accept(lex::LPAREN);
		lex::Lexeme oper;
		if(!p.accept(lex::LPAREN, lex::LBRACE)) {
			err::out(p.peek(), {"expected opening parenthesis/brace"
					    " for function/struct call, found: ",
					    p.peek().getTok().cStr()});
			return false;
		}
		p.sett(fncall ? lex::FNCALL : lex::STCALL);
		oper = p.peek();
		p.next();
		if(p.acceptn(fncall ? lex::RPAREN : lex::RBRACE)) {
			goto post_args;
		}
		// parse arguments
		while(true) {
			if(!parseExpr16(p, arg, false)) return false;
			args.push_back(arg);
			arg = nullptr;
			if(!p.acceptn(lex::COMMA)) break;
		}
		if(!p.acceptn(fncall ? lex::RPAREN : lex::RBRACE)) {
			err::out(p.peek(), {"expected closing parenthesis/brace after "
					    "function/struct call arguments, found: ",
					    p.peek().getTok().cStr()});
			return false;
		}
	post_args:
		rhs  = StmtFnArgs::create(ctx, oper.getLoc(), args);
		lhs  = StmtExpr::create(ctx, oper.getLoc(), lhs, oper, rhs);
		rhs  = nullptr;
		args = {};

		if(!disable_brace_after_iden && p.accept(lex::LBRACE)) goto begin_brack;
		if(p.accept(lex::LBRACK, lex::LPAREN)) goto begin_brack;
	}

dot:
	if(p.acceptn(lex::DOT, lex::ARROW)) {
		if(lhs && rhs) {
			lhs = StmtExpr::create(ctx, p.peek(-1).getLoc(), lhs, p.peek(-1), rhs);
			rhs = nullptr;
		}
		dot = p.peek(-1);
		goto after_dot;
	}

done:
	if(lhs && rhs) {
		lhs = StmtExpr::create(ctx, dot.getLoc(), lhs, dot, rhs);
		rhs = nullptr;
	}
	expr = lhs;
	return true;
}

bool Parser::parseVar(ParseHelper &p, StmtVar *&var, bool is_fn_arg)
{
	var = nullptr;

	bool is_const = false;
	if(p.acceptn(lex::CONST)) is_const = true;

	if(!p.accept(lex::IDEN)) {
		err::out(p.peek(), {"expected identifier for variable name, found: ",
				    p.peek().getTok().cStr()});
		return false;
	}
	lex::Lexeme &name = p.peek();
	p.next();
	Stmt *val      = nullptr;
	StmtSimple *in = nullptr;

	if(p.acceptn(lex::IN)) {
		if(!parseSimple(p, (Stmt *&)in)) {
			err::out(p.peek(),
				 {"failed to parse in-type for variable: ", name.getDataStr()});
			return false;
		}
	}

	if(!p.acceptn(lex::ASSN)) {
		if(is_fn_arg) goto end;
		err::out(name, {"invalid variable declaration - no value set"});
		return false;
	}
	if(!parseExpr16(p, val, false)) {
		return false;
	}

	if(in && val->getStmtType() != FNDEF) {
		err::out(name, {"only functions can be created using let-in statements"});
		return false;
	}

end:
	var = StmtVar::create(ctx, name.getLoc(), name, in, val, is_const);
	return true;
}

bool Parser::parseFnSig(ParseHelper &p, Stmt *&fsig)
{
	fsig = nullptr;

	Vector<StmtVar *> args;
	StmtVar *arg = nullptr;
	Set<StringRef> argnames;
	bool found_va	   = false;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::FN)) {
		err::out(p.peek(), {"expected 'fn' here, found: ", p.peek().getTok().cStr()});
		return false;
	}

	if(!p.acceptn(lex::LPAREN)) {
		err::out(p.peek(), {"expected opening parenthesis for function args, found: ",
				    p.peek().getTok().cStr()});
		return false;
	}
	if(p.acceptn(lex::RPAREN)) goto post_args;

	// args
	while(true) {
		if(argnames.find(p.peek().getDataStr()) != argnames.end()) {
			err::out(p.peek(), {"this argument name is already used "
					    "before in this function signature"});
			return false;
		}
		argnames.insert(p.peek().getDataStr());
		if(!parseVar(p, arg, true)) {
			err::out(p.peek(), {"failed to parse function definition parameter"});
			return false;
		}
		if(p.acceptn(lex::PostVA)) found_va = true;
		args.push_back(arg);
		arg = nullptr;
		if(!p.acceptn(lex::COMMA)) break;
		if(found_va) {
			err::out(p.peek(), {"no parameter can exist after variadic"});
			return false;
		}
	}

	if(!p.acceptn(lex::RPAREN)) {
		err::out(p.peek(), {"expected closing parenthesis after function args, found: ",
				    p.peek().getTok().cStr()});
		return false;
	}

post_args:
	fsig = StmtFnSig::create(ctx, start.getLoc(), args, found_va);
	return true;
}
bool Parser::parseFnDef(ParseHelper &p, Stmt *&fndef)
{
	fndef = nullptr;

	Stmt *sig	   = nullptr;
	StmtBlock *blk	   = nullptr;
	bool is_inline	   = false;
	lex::Lexeme &start = p.peek();

	if(!parseFnSig(p, sig)) return false;
	if(!parseBlock(p, blk)) return false;

	fndef = StmtFnDef::create(ctx, start.getLoc(), (StmtFnSig *)sig, blk);
	return true;
}

bool Parser::parseVardecl(ParseHelper &p, Stmt *&vd)
{
	vd = nullptr;

	Vector<StmtVar *> decls;
	StmtVar *decl	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::LET)) {
		err::out(p.peek(),
			 {"expected 'let' keyword here, found: ", p.peek().getTok().cStr()});
		return false;
	}

	while(p.accept(lex::IDEN, lex::CONST)) {
		if(!parseVar(p, decl, false)) return false;
		decls.push_back(decl);
		decl = nullptr;
		if(!p.acceptn(lex::COMMA)) break;
	}

	vd = StmtVarDecl::create(ctx, start.getLoc(), decls);
	return true;
}

bool Parser::parseConds(ParseHelper &p, Stmt *&conds)
{
	conds = nullptr;

	Vector<Conditional> cvec;
	Conditional c(nullptr, nullptr);

	lex::Lexeme &start = p.peek();

cond:
	if(!p.acceptn(lex::IF, lex::ELIF)) {
		err::out(p.peek(), {"expected 'if' here, found: ", p.peek().getTok().cStr()});
		return false;
	}

	if(!parseExpr15(p, c.getCond(), true)) {
		err::out(p.peek(), {"failed to parse condition for if/else if statement"});
		return false;
	}

blk:
	if(!parseBlock(p, c.getBlk())) {
		err::out(p.peek(), {"failed to parse block for conditional"});
		return false;
	}

	cvec.emplace_back(c.getCond(), c.getBlk());
	c.reset();

	if(p.accept(lex::ELIF)) goto cond;
	if(p.acceptn(lex::ELSE)) goto blk;

	conds = StmtCond::create(ctx, start.getLoc(), cvec);
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
// {
// let e_interm = vec.eachRev();
// for let _e = e_interm.begin(); _e != e_interm.end(); _e = e_interm.next(_e) {
// 	let e = e_interm.at(_e); // e is a reference
// 	...
// }
// }
bool Parser::parseForIn(ParseHelper &p, Stmt *&fin)
{
	fin = nullptr;

	lex::Lexeme iter;
	Stmt *in	   = nullptr; // L01
	StmtBlock *blk	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::FOR)) {
		err::out(p.peek(), {"expected 'for' here, found: ", p.peek().getTok().cStr()});
		return false;
	}

	if(!p.accept(lex::IDEN)) {
		err::out(p.peek(), {"expected iterator (identifier) here, found: ",
				    p.peek().getTok().cStr()});
		return false;
	}
	iter = p.peek();
	p.next();

	if(!p.acceptn(lex::IN)) {
		err::out(p.peek(), {"expected 'in' here, found: ", p.peek().getTok().cStr()});
		return false;
	}

	if(!parseExpr01(p, in, true)) {
		err::out(p.peek(), {"failed to parse expression for 'in'"});
		return false;
	}

	if(!p.accept(lex::LBRACE)) {
		err::out(p.peek(), {"expected block for for-in construct, found: ",
				    p.peek().getTok().cStr()});
		return false;
	}

	if(!parseBlock(p, blk)) {
		err::out(p.peek(), {"failed to parse block for for-in construct"});
		return false;
	}

	const ModuleLoc *loc	= iter.getLoc();
	lex::Lexeme in_interm	= iter; // e_interm
	lex::Lexeme iter_interm = iter; // _e
	lex::Lexeme lexbegin	= lex::Lexeme(loc, lex::IDEN, "begin");
	lex::Lexeme lexend	= lex::Lexeme(loc, lex::IDEN, "end");
	lex::Lexeme lexnext	= lex::Lexeme(loc, lex::IDEN, "next");
	lex::Lexeme lexat	= lex::Lexeme(loc, lex::IDEN, "at");
	lex::Lexeme dot_op	= lex::Lexeme(loc, lex::DOT);
	lex::Lexeme call_op	= lex::Lexeme(loc, lex::FNCALL);
	lex::Lexeme ne_op	= lex::Lexeme(loc, lex::NE);
	lex::Lexeme assn_op	= lex::Lexeme(loc, lex::ASSN);

	in_interm.setDataStr(ctx.strFrom({in_interm.getDataStr(), "_interm"}));
	iter_interm.setDataStr(ctx.strFrom({"_", iter_interm.getDataStr()}));

	StmtVar *in_interm_var =
	StmtVar::create(ctx, in_interm.getLoc(), in_interm, nullptr, in, 0);
	// block statement 1:
	StmtVarDecl *in_interm_vardecl = StmtVarDecl::create(ctx, loc, {in_interm_var});

	// init:
	// let <iter_interm> = <in_interm>.begin()
	StmtSimple *init_lhs	   = StmtSimple::create(ctx, loc, in_interm);
	StmtSimple *init_rhs	   = StmtSimple::create(ctx, loc, lexbegin);
	StmtExpr *init_dot_expr	   = StmtExpr::create(ctx, loc, init_lhs, dot_op, init_rhs);
	StmtFnArgs *init_call_info = StmtFnArgs::create(ctx, loc, {});
	StmtExpr *init_expr = StmtExpr::create(ctx, loc, init_dot_expr, call_op, init_call_info);
	StmtVar *init_iter_interm_var =
	StmtVar::create(ctx, iter_interm.getLoc(), iter_interm, nullptr, init_expr, 0);
	StmtVarDecl *init = StmtVarDecl::create(ctx, iter_interm.getLoc(), {init_iter_interm_var});

	// cond:
	// <iter_interm> != <in_interm>.end()
	StmtSimple *cond_rhs_lhs   = StmtSimple::create(ctx, loc, in_interm);
	StmtSimple *cond_rhs_rhs   = StmtSimple::create(ctx, loc, lexend);
	StmtExpr *cond_dot_expr	   = StmtExpr::create(ctx, loc, cond_rhs_lhs, dot_op, cond_rhs_rhs);
	StmtFnArgs *cond_call_info = StmtFnArgs::create(ctx, loc, {});
	StmtExpr *cond_rhs   = StmtExpr::create(ctx, loc, cond_dot_expr, call_op, cond_call_info);
	StmtSimple *cond_lhs = StmtSimple::create(ctx, iter_interm.getLoc(), iter_interm);
	StmtExpr *cond	     = StmtExpr::create(ctx, loc, cond_lhs, ne_op, cond_rhs);

	// incr:
	// <iter_interm> = <in_interm>.next(<iter_interm>)
	StmtSimple *incr_lhs_lhs   = StmtSimple::create(ctx, loc, in_interm);
	StmtSimple *incr_lhs_rhs   = StmtSimple::create(ctx, loc, lexnext);
	StmtExpr *incr_dot_expr	   = StmtExpr::create(ctx, loc, incr_lhs_lhs, dot_op, incr_lhs_rhs);
	StmtSimple *incr_call_arg  = StmtSimple::create(ctx, loc, iter_interm);
	StmtFnArgs *incr_call_info = StmtFnArgs::create(ctx, loc, {incr_call_arg});
	StmtExpr *incr_lhs   = StmtExpr::create(ctx, loc, incr_dot_expr, call_op, incr_call_info);
	StmtSimple *incr_rhs = StmtSimple::create(ctx, iter_interm.getLoc(), iter_interm);
	StmtExpr *incr	     = StmtExpr::create(ctx, loc, incr_lhs, assn_op, incr_rhs);

	// inside loop block:
	// let <iter> = <in_interm>.at(<iter_interm>)
	StmtSimple *loop_var_val_lhs = StmtSimple::create(ctx, loc, in_interm);
	StmtSimple *loop_var_val_rhs = StmtSimple::create(ctx, loc, lexat);
	StmtExpr *loop_var_val_dot_expr =
	StmtExpr::create(ctx, loc, loop_var_val_lhs, dot_op, loop_var_val_rhs);
	StmtSimple *loop_var_val_call_arg  = StmtSimple::create(ctx, loc, iter_interm);
	StmtFnArgs *loop_var_val_call_info = StmtFnArgs::create(ctx, loc, {loop_var_val_call_arg});
	StmtExpr *loop_var_val =
	StmtExpr::create(ctx, loc, loop_var_val_dot_expr, call_op, loop_var_val_call_info);
	StmtVar *loop_var	  = StmtVar::create(ctx, loc, iter, nullptr, loop_var_val, 0);
	StmtVarDecl *loop_vardecl = StmtVarDecl::create(ctx, loc, {loop_var});
	blk->getStmts().insert(blk->getStmts().begin(), loop_vardecl);

	// StmtFor - block statement 2:
	StmtFor *loop = StmtFor::create(ctx, start.getLoc(), init, cond, incr, blk);

	// StmtBlock
	Vector<Stmt *> outerblkstmts = {in_interm_vardecl, loop};
	fin			     = StmtBlock::create(ctx, start.getLoc(), outerblkstmts, false);
	return true;
}
bool Parser::parseFor(ParseHelper &p, Stmt *&f)
{
	f = nullptr;

	Stmt *init	   = nullptr; // either of StmtVarDecl or StmtExpr
	Stmt *cond	   = nullptr;
	Stmt *incr	   = nullptr;
	StmtBlock *blk	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::FOR)) {
		err::out(p.peek(), {"expected 'for' here, found: ", p.peek().getTok().cStr()});
		return false;
	}

init:
	if(p.acceptn(lex::COLS)) goto cond;

	if(p.accept(lex::LET)) {
		if(!parseVardecl(p, init)) return false;
	} else {
		if(!parseExpr(p, init, false)) return false;
	}
	if(!p.acceptn(lex::COLS)) {
		err::out(p.peek(), {"expected semicolon here, found: ", p.peek().getTok().cStr()});
		return false;
	}

cond:
	if(p.acceptn(lex::COLS)) goto incr;

	if(!parseExpr16(p, cond, false)) return false;
	if(!p.acceptn(lex::COLS)) {
		err::out(p.peek(), {"expected semicolon here, found: ", p.peek().getTok().cStr()});
		return false;
	}

incr:
	if(p.accept(lex::LBRACE)) goto body;

	if(!parseExpr(p, incr, true)) return false;
	if(!p.accept(lex::LBRACE)) {
		err::out(p.peek(),
			 {"expected braces for body here, found: ", p.peek().getTok().cStr()});
		return false;
	}

body:
	if(!parseBlock(p, blk)) {
		err::out(p.peek(), {"failed to parse block for 'for' construct"});
		return false;
	}

	f = StmtFor::create(ctx, start.getLoc(), init, cond, incr, blk);
	return true;
}
bool Parser::parseWhile(ParseHelper &p, Stmt *&w)
{
	w = nullptr;

	Stmt *cond	   = nullptr;
	StmtBlock *blk	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::WHILE)) {
		err::out(p.peek(), {"expected 'while' here, found: ", p.peek().getTok().cStr()});
		return false;
	}

	if(!parseExpr16(p, cond, true)) return false;

	if(!parseBlock(p, blk)) {
		err::out(p.peek(), {"failed to parse block for 'for' construct"});
		return false;
	}

	w = StmtFor::create(ctx, start.getLoc(), nullptr, cond, nullptr, blk);
	return true;
}
bool Parser::parseRet(ParseHelper &p, Stmt *&ret)
{
	ret = nullptr;

	Stmt *val	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::RETURN)) {
		err::out(p.peek(), {"expected 'return' here, found: ", p.peek().getTok().cStr()});
		return false;
	}

	if(p.accept(lex::COLS)) goto done;

	if(!parseExpr16(p, val, false)) {
		err::out(p.peek(), {"failed to parse expression for return value"});
		return false;
	}

done:
	ret = StmtRet::create(ctx, start.getLoc(), val);
	return true;
}
bool Parser::parseContinue(ParseHelper &p, Stmt *&cont)
{
	cont = nullptr;

	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::CONTINUE)) {
		err::out(p.peek(), {"expected 'continue' here, found: ", p.peek().getTok().cStr()});
		return false;
	}

	cont = StmtContinue::create(ctx, start.getLoc());
	return true;
}
bool Parser::parseBreak(ParseHelper &p, Stmt *&brk)
{
	brk = nullptr;

	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::BREAK)) {
		err::out(p.peek(), {"expected 'break' here, found: ", p.peek().getTok().cStr()});
		return false;
	}

	brk = StmtBreak::create(ctx, start.getLoc());
	return true;
}
bool Parser::parseDefer(ParseHelper &p, Stmt *&defer)
{
	defer = nullptr;

	Stmt *val	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::DEFER)) {
		err::out(p.peek(), {"expected 'defer' here, found: ", p.peek().getTok().cStr()});
		return false;
	}

	if(!parseExpr16(p, val, false)) {
		err::out(p.peek(), {"failed to parse expression for return value"});
		return false;
	}

done:
	defer = StmtDefer::create(ctx, start.getLoc(), val);
	return true;
}

} // namespace fer
