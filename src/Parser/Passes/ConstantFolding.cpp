// part of SimplifyPass

#include "AST/Passes/Simplify.hpp"

namespace fer::ast
{

template<typename T> T getValueAs(StmtSimple *s)
{
    T res = 0;
    switch(s->getTokType()) {
    case lex::FTRUE: res = 1; break;
    case lex::FFALSE: res = 0; break;
    case lex::INT: res = s->getDataInt(); break;
    case lex::FLT: res = s->getDataFlt(); break;
    default: res = 0;
    }
    return res;
}

#define binaryIntFltOps(OPER)                                                    \
    if(ltok == lex::INT && rtok == lex::INT) {                                   \
        int64_t res = l->getDataInt() OPER r->getDataInt();                      \
        resultStmt  = StmtSimple::create(allocator, l->getLoc(), lex::INT, res); \
    }                                                                            \
    if(ltok == lex::INT && rtok == lex::FLT) {                                   \
        double res = (double)l->getDataInt() OPER r->getDataFlt();               \
        resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::FLT, res);  \
    }                                                                            \
    if(ltok == lex::FLT && rtok == lex::INT) {                                   \
        double res = l->getDataFlt() OPER(double) r->getDataInt();               \
        resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::FLT, res);  \
    }                                                                            \
    if(ltok == lex::FLT && rtok == lex::FLT) {                                   \
        double res = l->getDataFlt() OPER r->getDataFlt();                       \
        resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::FLT, res);  \
    }

#define comparisonOps(OPER)                                                                   \
    if(ltok == lex::FLT || rtok == lex::FLT) {                                                \
        double lhs = getValueAs<double>(l);                                                   \
        double rhs = getValueAs<double>(r);                                                   \
        resultStmt = StmtSimple::create(allocator, l->getLoc(),                               \
                                        lhs OPER rhs ? lex::FTRUE : lex::FFALSE, (int64_t)0); \
    }                                                                                         \
    int64_t lhs = getValueAs<int64_t>(l);                                                     \
    int64_t rhs = getValueAs<int64_t>(r);                                                     \
    resultStmt  = StmtSimple::create(allocator, l->getLoc(),                                  \
                                    lhs OPER rhs ? lex::FTRUE : lex::FFALSE, (int64_t)0);

bool SimplifyPass::applyConstantFolding(Stmt *&resultStmt, StmtSimple *l, StmtSimple *r,
                                        lex::TokType oper)
{
    lex::TokType ltok = l->getTokType();
    lex::TokType rtok = r ? r->getTokType() : lex::INVALID;

    if(!l->getTok().isLiteral()) return true;
    if(r && !r->getTok().isLiteral()) return true;

    switch(oper) {
    case lex::ADD: {
        if(ltok == lex::STR && rtok == lex::STR) {
            String res(l->getDataStr());
            res += r->getDataStr();
            resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::STR, std::move(res));
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
            res.reserve(l->getDataStr().size() * r->getDataInt());
            for(int64_t i = 0; i < r->getDataInt(); ++i) { res += l->getDataStr(); }
            resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::STR, std::move(res));
        }
        // 2 * "xyz" = "xyzxyz"
        if(ltok == lex::INT && rtok == lex::STR) {
            String res;
            res.reserve(l->getDataInt() * r->getDataStr().size());
            for(int64_t i = 0; i < l->getDataInt(); ++i) { res += r->getDataStr(); }
            resultStmt = StmtSimple::create(allocator, r->getLoc(), lex::STR, std::move(res));
        }
        binaryIntFltOps(*);
        break;
    }
    case lex::DIV: {
        if((rtok == lex::INT && r->getDataInt() == 0) ||
           (rtok == lex::FLT && r->getDataFlt() == 0.0))
        {
            err.fail(l->getLoc(), "Attempted to divide by zero during constant folding pass");
            return false;
        }
        binaryIntFltOps(/);
        break;
    }
    case lex::MOD: {
        if(ltok == lex::INT && rtok == lex::INT) {
            int64_t res = l->getDataInt() % r->getDataInt();
            resultStmt  = StmtSimple::create(allocator, l->getLoc(), lex::INT, res);
        }
        break;
    }
    case lex::XINC: {
        if(ltok == lex::INT) {
            int64_t res = l->getDataInt();
            resultStmt  = StmtSimple::create(allocator, l->getLoc(), lex::INT, res++);
        }
        if(ltok == lex::FLT) {
            double res = l->getDataFlt();
            resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::FLT, res++);
        }
        break;
    }
    case lex::INCX: {
        if(ltok == lex::INT) {
            int64_t res = l->getDataInt();
            resultStmt  = StmtSimple::create(allocator, l->getLoc(), lex::INT, ++res);
        }
        if(ltok == lex::FLT) {
            double res = l->getDataFlt();
            resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::FLT, ++res);
        }
        break;
    }
    case lex::XDEC: {
        if(ltok == lex::INT) {
            int64_t res = l->getDataInt();
            resultStmt  = StmtSimple::create(allocator, l->getLoc(), lex::INT, res--);
        }
        if(ltok == lex::FLT) {
            double res = l->getDataFlt();
            resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::FLT, res--);
        }
        break;
    }
    case lex::DECX: {
        if(ltok == lex::INT) {
            int64_t res = l->getDataInt();
            resultStmt  = StmtSimple::create(allocator, l->getLoc(), lex::INT, --res);
        }
        if(ltok == lex::FLT) {
            double res = l->getDataFlt();
            resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::FLT, --res);
        }
        break;
    }
    case lex::UADD: {
        if(ltok == lex::INT) {
            int64_t res = l->getDataInt();
            resultStmt  = StmtSimple::create(allocator, l->getLoc(), lex::INT, +res);
        }
        if(ltok == lex::FLT) {
            double res = l->getDataFlt();
            resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::FLT, +res);
        }
        break;
    }
    case lex::USUB: {
        if(ltok == lex::INT) {
            int64_t res = l->getDataInt();
            resultStmt  = StmtSimple::create(allocator, l->getLoc(), lex::INT, -res);
        }
        if(ltok == lex::FLT) {
            double res = l->getDataFlt();
            resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::FLT, -res);
        }
        break;
    }
    case lex::LAND: {
        if(ltok == lex::INT && rtok == lex::INT) {
            lex::TokType res = l->getDataInt() && r->getDataInt() ? lex::FTRUE : lex::FFALSE;
            resultStmt       = StmtSimple::create(allocator, l->getLoc(), res, (int64_t)0);
        }
        if(ltok == lex::INT && rtok == lex::FLT) {
            lex::TokType res =
                (double)l->getDataInt() && r->getDataFlt() ? lex::FTRUE : lex::FFALSE;
            resultStmt = StmtSimple::create(allocator, l->getLoc(), res, (int64_t)0);
        }
        if(ltok == lex::FLT && rtok == lex::INT) {
            lex::TokType res =
                l->getDataFlt() && (double)r->getDataInt() ? lex::FTRUE : lex::FFALSE;
            resultStmt = StmtSimple::create(allocator, l->getLoc(), res, (int64_t)0);
        }
        if(ltok == lex::FLT && rtok == lex::FLT) {
            lex::TokType res = l->getDataFlt() && r->getDataFlt() ? lex::FTRUE : lex::FFALSE;
            resultStmt       = StmtSimple::create(allocator, l->getLoc(), res, (int64_t)0);
        }

        // with booleans
        if(ltok == lex::FFALSE || rtok == lex::FFALSE) {
            resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::FFALSE, (int64_t)0);
        }
        if(ltok == lex::FTRUE && rtok == lex::FTRUE) {
            resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::FTRUE, (int64_t)0);
        }
        if((ltok == lex::FTRUE && rtok == lex::FFALSE) ||
           (ltok == lex::FFALSE && rtok == lex::FTRUE))
        {
            resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::FFALSE, (int64_t)0);
        }
        if(ltok == lex::INT && rtok == lex::FTRUE) {
            // && true is not required
            lex::TokType res = l->getDataInt() ? lex::FTRUE : lex::FFALSE;
            resultStmt       = StmtSimple::create(allocator, l->getLoc(), res, (int64_t)0);
        }
        if(ltok == lex::FLT && rtok == lex::FTRUE) {
            // && true is not required
            lex::TokType res = l->getDataInt() ? lex::FTRUE : lex::FFALSE;
            resultStmt       = StmtSimple::create(allocator, l->getLoc(), res, (int64_t)0);
        }
        break;
    }
    case lex::LOR: {
        if(ltok == lex::INT && rtok == lex::INT) {
            lex::TokType res = l->getDataInt() || r->getDataInt() ? lex::FTRUE : lex::FFALSE;
            resultStmt       = StmtSimple::create(allocator, l->getLoc(), res, (int64_t)0);
        }
        if(ltok == lex::INT && rtok == lex::FLT) {
            lex::TokType res =
                (double)l->getDataInt() || r->getDataFlt() ? lex::FTRUE : lex::FFALSE;
            resultStmt = StmtSimple::create(allocator, l->getLoc(), res, (int64_t)0);
        }
        if(ltok == lex::FLT && rtok == lex::INT) {
            lex::TokType res =
                l->getDataFlt() || (double)r->getDataInt() ? lex::FTRUE : lex::FFALSE;
            resultStmt = StmtSimple::create(allocator, l->getLoc(), res, (int64_t)0);
        }
        if(ltok == lex::FLT && rtok == lex::FLT) {
            lex::TokType res = l->getDataFlt() || r->getDataFlt() ? lex::FTRUE : lex::FFALSE;
            resultStmt       = StmtSimple::create(allocator, l->getLoc(), res, (int64_t)0);
        }

        // with booleans
        if(ltok == lex::FTRUE || rtok == lex::FTRUE) {
            resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::FTRUE, (int64_t)0);
        }
        if(ltok == lex::FFALSE && rtok == lex::FFALSE) {
            resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::FFALSE, (int64_t)0);
        }
        if((ltok == lex::FTRUE && rtok == lex::FFALSE) ||
           (ltok == lex::FFALSE && rtok == lex::FTRUE))
        {
            resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::FTRUE, (int64_t)0);
        }
        if(ltok == lex::INT && rtok == lex::FFALSE) {
            // || false is not required
            lex::TokType res = l->getDataInt() ? lex::FTRUE : lex::FFALSE;
            resultStmt       = StmtSimple::create(allocator, l->getLoc(), res, (int64_t)0);
        }
        if(ltok == lex::FLT && rtok == lex::FFALSE) {
            // || false is not required
            lex::TokType res = l->getDataInt() ? lex::FTRUE : lex::FFALSE;
            resultStmt       = StmtSimple::create(allocator, l->getLoc(), res, (int64_t)0);
        }
        break;
    }
    case lex::LNOT: {
        if(ltok == lex::INT) {
            resultStmt = StmtSimple::create(allocator, l->getLoc(),
                                            l->getDataInt() ? lex::FFALSE : lex::FTRUE, (int64_t)0);
        }
        if(ltok == lex::FLT) {
            resultStmt = StmtSimple::create(allocator, l->getLoc(),
                                            l->getDataFlt() ? lex::FFALSE : lex::FTRUE, (int64_t)0);
        }
        if(ltok == lex::FTRUE) {
            resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::FFALSE, (int64_t)0);
        }
        if(ltok == lex::FFALSE) {
            resultStmt = StmtSimple::create(allocator, l->getLoc(), lex::FTRUE, (int64_t)0);
        }
        break;
    }
    case lex::EQ: {
        if(ltok == lex::STR && rtok == lex::STR) {
            bool res   = l->getDataStr() == r->getDataStr();
            resultStmt = StmtSimple::create(allocator, l->getLoc(), res ? lex::FTRUE : lex::FFALSE,
                                            (int64_t)0);
        }
        if((ltok == lex::STR || rtok == lex::STR) && ltok != rtok) break;
        comparisonOps(==);
        break;
    }
    case lex::NE: {
        if(ltok == lex::STR && rtok == lex::STR) {
            bool res   = l->getDataStr() == r->getDataStr();
            resultStmt = StmtSimple::create(allocator, l->getLoc(), res ? lex::FTRUE : lex::FFALSE,
                                            (int64_t)0);
        }
        if((ltok == lex::STR || rtok == lex::STR) && ltok != rtok) break;
        comparisonOps(!=);
        break;
    }
    case lex::LT: {
        if(ltok == lex::STR || rtok == lex::STR) break;
        comparisonOps(<);
        break;
    }
    case lex::GT: {
        if(ltok == lex::STR || rtok == lex::STR) break;
        comparisonOps(>);
        break;
    }
    case lex::LE: {
        if(ltok == lex::STR || rtok == lex::STR) break;
        comparisonOps(<=);
        break;
    }
    case lex::GE: {
        if(ltok == lex::STR || rtok == lex::STR) break;
        comparisonOps(>=);
        break;
    }
    case lex::BAND: {
        if(ltok == lex::STR || rtok == lex::STR) break;
        if(ltok == lex::FLT || rtok == lex::FLT) break;
        int64_t res = getValueAs<int64_t>(l) & getValueAs<int64_t>(r);
        resultStmt  = StmtSimple::create(allocator, l->getLoc(), lex::INT, res);
        break;
    }
    case lex::BOR: {
        if(ltok == lex::STR || rtok == lex::STR) break;
        if(ltok == lex::FLT || rtok == lex::FLT) break;
        int64_t res = getValueAs<int64_t>(l) | getValueAs<int64_t>(r);
        resultStmt  = StmtSimple::create(allocator, l->getLoc(), lex::INT, res);
        break;
    }
    case lex::BNOT: {
        if(ltok == lex::STR || ltok == lex::FLT) break;
        int64_t res = ~getValueAs<int64_t>(l);
        resultStmt  = StmtSimple::create(allocator, l->getLoc(), lex::INT, res);
        break;
    }
    case lex::BXOR: {
        if(ltok == lex::STR || rtok == lex::STR) break;
        if(ltok == lex::FLT || rtok == lex::FLT) break;
        int64_t res = getValueAs<int64_t>(l) ^ getValueAs<int64_t>(r);
        resultStmt  = StmtSimple::create(allocator, l->getLoc(), lex::INT, res);
        break;
    }
    case lex::LSHIFT: {
        if(ltok == lex::STR || rtok == lex::STR) break;
        if(ltok == lex::FLT || rtok == lex::FLT) break;
        int64_t res = getValueAs<int64_t>(l) << getValueAs<int64_t>(r);
        resultStmt  = StmtSimple::create(allocator, l->getLoc(), lex::INT, res);
        break;
    }
    case lex::RSHIFT: {
        if(ltok == lex::STR || rtok == lex::STR) break;
        if(ltok == lex::FLT || rtok == lex::FLT) break;
        int64_t res = getValueAs<int64_t>(l) >> getValueAs<int64_t>(r);
        resultStmt  = StmtSimple::create(allocator, l->getLoc(), lex::INT, res);
        break;
    }
    default: break;
    }

    return true;
}

} // namespace fer::ast