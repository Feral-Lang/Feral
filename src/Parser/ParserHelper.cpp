#include "AST/ParseHelper.hpp"

namespace fer::ast
{

ParseHelper::ParseHelper(const ManagedList &toks, lex::Lexeme *curr)
    : toks(toks), invalid({}, lex::INVALID), eof({}, lex::FEOF),
      curr(curr ? curr : (lex::Lexeme *)toks.getStart())
{}

lex::Lexeme *ParseHelper::getAt(lex::Lexeme *from, int offset)
{
    lex::Lexeme *iter = from;
    while(offset < 0 && iter) {
        iter = (lex::Lexeme *)toks.prev(iter);
        ++offset;
    }
    while(offset > 0 && iter) {
        iter = (lex::Lexeme *)toks.next(iter);
        --offset;
    }
    return iter ? iter : &eof;
}

lex::TokType ParseHelper::peekt(int offset)
{
    lex::Lexeme *res = peek(offset);
    return res->getTokVal();
}

lex::TokType ParseHelper::nextt()
{
    next();
    return peekt();
}

lex::TokType ParseHelper::prevt()
{
    prev();
    return peekt();
}

lex::Lexeme *ParseHelper::at(size_t idx) const { return (lex::Lexeme *)toks.at(idx); }

} // namespace fer::ast