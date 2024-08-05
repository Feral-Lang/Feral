// part of SimplifyPass

#include "Parser/Passes/Simplify.hpp"

namespace fer
{

template<typename T> T getValueAs(const lex::Lexeme &tok)
{
	T res = 0;
	switch(tok.getTokVal()) {
	case lex::FTRUE: res = 1; break;
	case lex::FFALSE: res = 0; break;
	case lex::INT: res = tok.getDataInt(); break;
	case lex::FLT: res = tok.getDataFlt(); break;
	default: res = 0;
	}
	return res;
}

#define binaryIntFltOps(OPER)                                                              \
	if(ltok == lex::INT && rtok == lex::INT) {                                         \
		int64_t res = l->getLexDataInt() OPER r->getLexDataInt();                  \
		lex::Lexeme restok(l->getLoc(), res);                                      \
		return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);                 \
	}                                                                                  \
	if(ltok == lex::INT && rtok == lex::FLT) {                                         \
		long double res = (long double)l->getLexDataInt() OPER r->getLexDataFlt(); \
		lex::Lexeme restok(l->getLoc(), res);                                      \
		return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);                 \
	}                                                                                  \
	if(ltok == lex::FLT && rtok == lex::INT) {                                         \
		long double res = l->getLexDataFlt() OPER(long double) r->getLexDataInt(); \
		lex::Lexeme restok(l->getLoc(), res);                                      \
		return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);                 \
	}                                                                                  \
	if(ltok == lex::FLT && rtok == lex::FLT) {                                         \
		long double res = l->getLexDataFlt() OPER r->getLexDataFlt();              \
		lex::Lexeme restok(l->getLoc(), res);                                      \
		return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);                 \
	}

#define comparisonOps(OPER)                                                               \
	if(ltok == lex::FLT || rtok == lex::FLT) {                                        \
		long double lhs = getValueAs<long double>(l->getLexValue());              \
		long double rhs = getValueAs<long double>(r->getLexValue());              \
		lex::Lexeme restok(l->getLoc(), lhs OPER rhs ? lex::FTRUE : lex::FFALSE); \
		return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);                \
	}                                                                                 \
	int64_t lhs = getValueAs<int64_t>(l->getLexValue());                              \
	int64_t rhs = getValueAs<int64_t>(r->getLexValue());                              \
	lex::Lexeme restok(l->getLoc(), lhs OPER rhs ? lex::FTRUE : lex::FFALSE);         \
	return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);

Stmt *SimplifyPass::applyConstantFolding(StmtSimple *l, StmtSimple *r, const lex::Tok &oper)
{
	lex::TokType ltok = l->getLexValue().getTokVal();
	lex::TokType rtok = r ? r->getLexValue().getTokVal() : lex::INVALID;

	if(!l->getLexValue().getTok().isLiteral()) return nullptr;
	if(r && !r->getLexValue().getTok().isLiteral()) return nullptr;

	switch(oper.getVal()) {
	case lex::ADD: {
		if(ltok == lex::STR && rtok == lex::STR) {
			lex::Lexeme restok(l->getLoc(), lex::STR, StringRef());
			restok.setDataStr({l->getLexDataStr(), r->getLexDataStr()});
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		binaryIntFltOps(+);
		break;
	}
	case lex::SUB: {
		binaryIntFltOps(-);
		break;
	}
	case lex::MUL: {
		// "xyz" * 2 = "xyzxyz"
		if(ltok == lex::STR && rtok == lex::INT) {
			String res;
			res.reserve(l->getLexDataStr().size() * r->getLexDataInt());
			for(int64_t i = 0; i < r->getLexDataInt(); ++i) {
				res += l->getLexDataStr();
			}
			lex::Lexeme restok(l->getLoc(), lex::STR, StringRef());
			restok.setDataStr(res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		// 2 * "xyz" = "xyzxyz"
		if(ltok == lex::INT && rtok == lex::STR) {
			String res;
			res.reserve(l->getLexDataInt() * r->getLexDataStr().size());
			for(int64_t i = 0; i < l->getLexDataInt(); ++i) {
				res += r->getLexDataStr();
			}
			lex::Lexeme restok(l->getLoc(), lex::STR, StringRef());
			restok.setDataStr(res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		binaryIntFltOps(*);
		break;
	}
	case lex::DIV: {
		binaryIntFltOps(/);
		break;
	}
	case lex::MOD: {
		if(ltok == lex::INT && rtok == lex::INT) {
			int64_t res = l->getLexDataInt() % r->getLexDataInt();
			lex::Lexeme restok(l->getLoc(), res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		break;
	}
	case lex::XINC: {
		if(ltok == lex::INT) {
			int64_t res = l->getLexDataInt();
			lex::Lexeme restok(l->getLoc(), res++);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::FLT) {
			long double res = l->getLexDataFlt();
			lex::Lexeme restok(l->getLoc(), res++);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		break;
	}
	case lex::INCX: {
		if(ltok == lex::INT) {
			int64_t res = l->getLexDataInt();
			lex::Lexeme restok(l->getLoc(), ++res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::FLT) {
			long double res = l->getLexDataFlt();
			lex::Lexeme restok(l->getLoc(), ++res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		break;
	}
	case lex::XDEC: {
		if(ltok == lex::INT) {
			int64_t res = l->getLexDataInt();
			lex::Lexeme restok(l->getLoc(), res--);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::FLT) {
			long double res = l->getLexDataFlt();
			lex::Lexeme restok(l->getLoc(), res--);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		break;
	}
	case lex::DECX: {
		if(ltok == lex::INT) {
			int64_t res = l->getLexDataInt();
			lex::Lexeme restok(l->getLoc(), --res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::FLT) {
			long double res = l->getLexDataFlt();
			lex::Lexeme restok(l->getLoc(), --res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		break;
	}
	case lex::UADD: {
		if(ltok == lex::INT) {
			int64_t res = l->getLexDataInt();
			lex::Lexeme restok(l->getLoc(), +res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::FLT) {
			long double res = l->getLexDataFlt();
			lex::Lexeme restok(l->getLoc(), +res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		break;
	}
	case lex::USUB: {
		if(ltok == lex::INT) {
			int64_t res = l->getLexDataInt();
			lex::Lexeme restok(l->getLoc(), -res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::FLT) {
			long double res = l->getLexDataFlt();
			lex::Lexeme restok(l->getLoc(), -res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		break;
	}
	case lex::LAND: {
		if(ltok == lex::INT && rtok == lex::INT) {
			lex::TokType res =
			l->getLexDataInt() && r->getLexDataInt() ? lex::FTRUE : lex::FFALSE;
			lex::Lexeme restok(l->getLoc(), res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::INT && rtok == lex::FLT) {
			lex::TokType res = (long double)l->getLexDataInt() && r->getLexDataFlt()
					   ? lex::FTRUE
					   : lex::FFALSE;
			lex::Lexeme restok(l->getLoc(), res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::FLT && rtok == lex::INT) {
			lex::TokType res = l->getLexDataFlt() && (long double)r->getLexDataInt()
					   ? lex::FTRUE
					   : lex::FFALSE;
			lex::Lexeme restok(l->getLoc(), res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::FLT && rtok == lex::FLT) {
			lex::TokType res =
			l->getLexDataFlt() && r->getLexDataFlt() ? lex::FTRUE : lex::FFALSE;
			lex::Lexeme restok(l->getLoc(), res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}

		// with booleans
		if(ltok == lex::FFALSE || rtok == lex::FFALSE) {
			lex::Lexeme restok(l->getLoc(), lex::FFALSE);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::FTRUE && rtok == lex::FTRUE) {
			lex::Lexeme restok(l->getLoc(), lex::FTRUE);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if((ltok == lex::FTRUE && rtok == lex::FFALSE) ||
		   (ltok == lex::FFALSE && rtok == lex::FTRUE))
		{
			lex::Lexeme restok(l->getLoc(), lex::FFALSE);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::INT && rtok == lex::FTRUE) {
			// && true is not required
			lex::TokType res = l->getLexDataInt() ? lex::FTRUE : lex::FFALSE;
			lex::Lexeme restok(l->getLoc(), res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::FLT && rtok == lex::FTRUE) {
			// && true is not required
			lex::TokType res = l->getLexDataInt() ? lex::FTRUE : lex::FFALSE;
			lex::Lexeme restok(l->getLoc(), res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		break;
	}
	case lex::LOR: {
		if(ltok == lex::INT && rtok == lex::INT) {
			lex::TokType res =
			l->getLexDataInt() || r->getLexDataInt() ? lex::FTRUE : lex::FFALSE;
			lex::Lexeme restok(l->getLoc(), res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::INT && rtok == lex::FLT) {
			lex::TokType res = (long double)l->getLexDataInt() || r->getLexDataFlt()
					   ? lex::FTRUE
					   : lex::FFALSE;
			lex::Lexeme restok(l->getLoc(), res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::FLT && rtok == lex::INT) {
			lex::TokType res = l->getLexDataFlt() || (long double)r->getLexDataInt()
					   ? lex::FTRUE
					   : lex::FFALSE;
			lex::Lexeme restok(l->getLoc(), res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::FLT && rtok == lex::FLT) {
			lex::TokType res =
			l->getLexDataFlt() || r->getLexDataFlt() ? lex::FTRUE : lex::FFALSE;
			lex::Lexeme restok(l->getLoc(), res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}

		// with booleans
		if(ltok == lex::FTRUE || rtok == lex::FTRUE) {
			lex::Lexeme restok(l->getLoc(), lex::FTRUE);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::FFALSE && rtok == lex::FFALSE) {
			lex::Lexeme restok(l->getLoc(), lex::FFALSE);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if((ltok == lex::FTRUE && rtok == lex::FFALSE) ||
		   (ltok == lex::FFALSE && rtok == lex::FTRUE))
		{
			lex::Lexeme restok(l->getLoc(), lex::FTRUE);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::INT && rtok == lex::FFALSE) {
			// || false is not required
			lex::TokType res = l->getLexDataInt() ? lex::FTRUE : lex::FFALSE;
			lex::Lexeme restok(l->getLoc(), res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::FLT && rtok == lex::FFALSE) {
			// || false is not required
			lex::TokType res = l->getLexDataInt() ? lex::FTRUE : lex::FFALSE;
			lex::Lexeme restok(l->getLoc(), res);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		break;
	}
	case lex::LNOT: {
		if(ltok == lex::INT) {
			lex::Lexeme restok(l->getLoc(),
					   l->getLexDataInt() ? lex::FFALSE : lex::FTRUE);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::FLT) {
			lex::Lexeme restok(l->getLoc(),
					   l->getLexDataFlt() ? lex::FFALSE : lex::FTRUE);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::FTRUE) {
			lex::Lexeme restok(l->getLoc(), lex::FFALSE);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if(ltok == lex::FFALSE) {
			lex::Lexeme restok(l->getLoc(), lex::FTRUE);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		break;
	}
	case lex::EQ: {
		if(ltok == lex::STR && rtok == lex::STR) {
			bool res = l->getLexDataStr() == r->getLexDataStr();
			lex::Lexeme restok(l->getLoc(), res ? lex::FTRUE : lex::FFALSE);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if((ltok == lex::STR || rtok == lex::STR) && ltok != rtok) break;
		comparisonOps(==);
	}
	case lex::NE: {
		if(ltok == lex::STR && rtok == lex::STR) {
			bool res = l->getLexDataStr() == r->getLexDataStr();
			lex::Lexeme restok(l->getLoc(), res ? lex::FTRUE : lex::FFALSE);
			return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
		}
		if((ltok == lex::STR || rtok == lex::STR) && ltok != rtok) break;
		comparisonOps(!=);
	}
	case lex::LT: {
		if(ltok == lex::STR || rtok == lex::STR) break;
		comparisonOps(<);
	}
	case lex::GT: {
		if(ltok == lex::STR || rtok == lex::STR) break;
		comparisonOps(>);
	}
	case lex::LE: {
		if(ltok == lex::STR || rtok == lex::STR) break;
		comparisonOps(<=);
	}
	case lex::GE: {
		if(ltok == lex::STR || rtok == lex::STR) break;
		comparisonOps(>=);
	}
	case lex::BAND: {
		if(ltok == lex::STR || rtok == lex::STR) break;
		if(ltok == lex::FLT || rtok == lex::FLT) break;
		int64_t res =
		getValueAs<int64_t>(l->getLexValue()) & getValueAs<int64_t>(r->getLexValue());
		lex::Lexeme restok(l->getLoc(), res);
		return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
	}
	case lex::BOR: {
		if(ltok == lex::STR || rtok == lex::STR) break;
		if(ltok == lex::FLT || rtok == lex::FLT) break;
		int64_t res =
		getValueAs<int64_t>(l->getLexValue()) | getValueAs<int64_t>(r->getLexValue());
		lex::Lexeme restok(l->getLoc(), res);
		return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
	}
	case lex::BNOT: {
		if(ltok == lex::STR || ltok == lex::FLT) break;
		int64_t res = ~getValueAs<int64_t>(l->getLexValue());
		lex::Lexeme restok(l->getLoc(), res);
		return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
	}
	case lex::BXOR: {
		if(ltok == lex::STR || rtok == lex::STR) break;
		if(ltok == lex::FLT || rtok == lex::FLT) break;
		int64_t res =
		getValueAs<int64_t>(l->getLexValue()) ^ getValueAs<int64_t>(r->getLexValue());
		lex::Lexeme restok(l->getLoc(), res);
		return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
	}
	case lex::LSHIFT: {
		if(ltok == lex::STR || rtok == lex::STR) break;
		if(ltok == lex::FLT || rtok == lex::FLT) break;
		int64_t res = getValueAs<int64_t>(l->getLexValue())
			      << getValueAs<int64_t>(r->getLexValue());
		lex::Lexeme restok(l->getLoc(), res);
		return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
	}
	case lex::RSHIFT: {
		if(ltok == lex::STR || rtok == lex::STR) break;
		if(ltok == lex::FLT || rtok == lex::FLT) break;
		int64_t res =
		getValueAs<int64_t>(l->getLexValue()) >> getValueAs<int64_t>(r->getLexValue());
		lex::Lexeme restok(l->getLoc(), res);
		return ctx.allocStmt<StmtSimple>(restok.getLoc(), restok);
	}
	default: break;
	}

	return nullptr;
}

} // namespace fer