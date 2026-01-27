#pragma once

#include "Error.hpp"

namespace fer::lex
{

enum TokType
{
    INT,
    FLT,

    STR,
    IDEN,

    // Keywords
    LET,
    FN,
    IF,
    ELIF,
    ELSE,
    FOR,
    FIN, // IN but Windows APIs define a macro with that name
    WHILE,
    RETURN,
    CONTINUE,
    BREAK,
    FVOID,  // VOID but Windows APIs define a macro with that name
    FTRUE,  // TRUE but Windows APIs define a macro with that name
    FFALSE, // FALSE but Windows APIs define a macro with that name
    NIL,
    OR,
    FCONST, // CONST but Windows APIs define a macro with that name
    DEFER,
    INLINE,

    // Operators
    ASSN,
    // Arithmetic
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    ADD_ASSN,
    SUB_ASSN,
    MUL_ASSN,
    DIV_ASSN,
    MOD_ASSN,
    POWER,             // **
    ROOT,              // //
    NIL_COALESCE,      // ??
    NIL_COALESCE_ASSN, // ??=
    // Post/Pre Inc/Dec
    XINC,
    INCX,
    XDEC,
    DECX,
    // Unary
    UADD,
    USUB,
    UAND, // address of
    UMUL, // dereference
    // Logic
    LAND,
    LOR,
    LNOT,
    // Comparison
    EQ,
    LT,
    GT,
    LE,
    GE,
    NE,
    // Bitwise
    BAND,
    BOR,
    BNOT,
    BXOR,
    BAND_ASSN,
    BOR_ASSN,
    BNOT_ASSN,
    BXOR_ASSN,
    // Others
    LSHIFT,
    RSHIFT,
    LSHIFT_ASSN,
    RSHIFT_ASSN,

    SUBS,

    FNCALL, // function call and struct template specialization

    // Varargs
    PreVA,
    PostVA,

    // Separators
    DOT,
    QUEST,
    COL,
    COMMA,
    AT,
    SPC,
    TAB,
    NEWL,
    COLS, // Semi colon
    ARROW,
    // Parenthesis, Braces, Brackets
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACK,
    RBRACK,

    FEOF,
    INVALID,

    _LAST,
};

/**
 * \brief String value of each of the lexical tokens
 */
extern const char *TokStrs[_LAST];

class Tok
{
    TokType val;

public:
    Tok(int tok);

    inline bool isData() const
    {
        return val == INT || val == FLT || val == STR || val == IDEN || val == FVOID ||
               val == FTRUE || val == FFALSE || val == NIL;
    }
    inline bool isLiteral() const
    {
        return val == INT || val == FLT || val == STR || val == FTRUE || val == FFALSE;
    }

    inline bool isOper() const { return val >= ASSN && val <= RBRACK; }

    inline bool isUnaryPre() const
    {
        return val == UADD || val == USUB || val == UAND || val == UMUL || val == INCX ||
               val == DECX || val == LNOT || val == BNOT;
    }

    inline bool isUnaryPost() const { return val == XINC || val == XDEC; }

    inline bool isComparison() const
    {
        return val == EQ || val == LT || val == GT || val == LE || val == GE || val == NE;
    }

    inline bool isAssign() const
    {
        return (val == ASSN || val == ADD_ASSN || val == SUB_ASSN || val == MUL_ASSN ||
                val == DIV_ASSN || val == MOD_ASSN || val == BAND_ASSN || val == BOR_ASSN ||
                val == BNOT_ASSN || val == BXOR_ASSN || val == LSHIFT_ASSN || val == RSHIFT_ASSN);
    }

    inline bool isValid() const { return val != INVALID && val != FEOF; }

    inline const char *cStr() const { return TokStrs[val]; }
    inline String str() const { return TokStrs[val]; }

    inline bool operator==(Tok other) const { return val == other.val; }

    inline TokType getVal() const { return val; }

    inline void setVal(TokType v) { val = v; }

    inline bool isType(TokType other) const { return val == other; }
};

class Lexeme : public IAllocated
{
public:
    using Data = Variant<String, StringRef, int64_t, double>;

private:
    Data data;
    ModuleLoc loc;
    Tok tok;

public:
    Lexeme(ModuleLoc loc = {});
    explicit Lexeme(ModuleLoc loc, TokType type);
    explicit Lexeme(ModuleLoc loc, TokType type, String &&_data);
    explicit Lexeme(ModuleLoc loc, TokType type, StringRef _data);
    explicit Lexeme(ModuleLoc loc, int64_t _data);
    explicit Lexeme(ModuleLoc loc, double _data);

    bool cmpData(const Lexeme &other, TokType type) const;

    String str(int64_t pad = 10) const;

    inline bool operator==(const Lexeme &other) const
    {
        return tok == other.tok && cmpData(other, tok.getVal());
    }
    inline bool operator!=(const Lexeme &other) const { return *this == other ? false : true; }

    inline void setDataStr(String &&str) { data = std::move(str); }
    inline void setDataStr(StringRef str) { data = str; }
    inline void setDataInt(int64_t i) { data = i; }
    inline void setDataFlt(double f) { data = f; }
    template<typename... Args> void setDataStr(Args... args)
    {
        data = utils::toString(std::forward<Args>(args)...);
    }

    inline void appendDataStr(StringRef str)
    {
        if(!hasString()) data = String(getDataStr());
        std::get<String>(data) += str;
    }

    inline String getMoveDataStr()
    {
        return hasString() ? std::move(std::get<String>(data)) : String(std::get<StringRef>(data));
    }

    inline StringRef getDataStr() const
    {
        return hasString() ? std::get<String>(data) : std::get<StringRef>(data);
    }
    inline int64_t getDataInt() const { return std::get<int64_t>(data); }
    inline double getDataFlt() const { return std::get<double>(data); }

    inline Tok &getTok() { return tok; }
    inline const Tok &getTok() const { return tok; }
    inline TokType getTokVal() const { return tok.getVal(); }
    inline ModuleLoc getLoc() const { return loc; }

    inline bool hasString() const { return std::holds_alternative<String>(data); }
};

bool tokenize(ModuleId moduleId, StringRef path, StringRef data, ManagedList &toks);
void dumpTokens(OStream &os, const ManagedList &toks);

} // namespace fer::lex