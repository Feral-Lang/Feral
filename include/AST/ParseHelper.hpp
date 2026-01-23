#pragma once

#include "Lexer.hpp"

namespace fer::ast
{

class ParseHelper
{
    // requires modification at parsing stage, hence not set as const
    const ManagedList &toks;
    lex::Lexeme invalid, eof;
    lex::Lexeme *curr;

    lex::Lexeme *getAt(lex::Lexeme *from, int offset);

public:
    ParseHelper(const ManagedList &toks, lex::Lexeme *curr = 0);

    // never returns nullptr - returns either valid data or eof
    inline lex::Lexeme *peek(int offset = 0) { return getAt(curr, offset); }
    lex::TokType peekt(int offset = 0);

    // never returns nullptr - returns either valid data or eof
    inline lex::Lexeme *next() { return curr = getAt(curr, 1); }
    lex::TokType nextt();

    // never returns nullptr - returns either valid data or eof
    inline lex::Lexeme *prev() { return curr = getAt(curr, -1); }
    lex::TokType prevt();

    inline void sett(lex::TokType type)
    {
        if(curr) curr->getTok().setVal(type);
    }

    inline bool accept(lex::TokType type) { return peekt() == type; }
    inline bool accept(lex::TokType t1, lex::TokType t2)
    {
        lex::TokType t = peekt();
        return t == t1 || t == t2;
    }
    inline bool accept(lex::TokType t1, lex::TokType t2, lex::TokType t3)
    {
        lex::TokType t = peekt();
        return t == t1 || t == t2 || t == t3;
    }

    inline bool acceptn(lex::TokType type)
    {
        if(!accept(type)) return false;
        next();
        return true;
    }
    inline bool acceptn(lex::TokType t1, lex::TokType t2)
    {
        if(!accept(t1, t2)) return false;
        next();
        return true;
    }
    inline bool acceptn(lex::TokType t1, lex::TokType t2, lex::TokType t3)
    {
        if(!accept(t1, t2, t3)) return false;
        next();
        return true;
    }

    inline bool acceptd() { return peek()->getTok().isData(); }
    inline bool isValid() { return curr && !accept(lex::INVALID, lex::FEOF); }
    inline bool hasNext() { return peek(1) != nullptr; }

    lex::Lexeme *at(size_t idx) const;
};

} // namespace fer::ast