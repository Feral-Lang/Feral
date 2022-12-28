#pragma once

#include "Module.hpp"

namespace fer::lex
{

enum TokType
{
	INT,
	FLT,

	CHAR,
	STR,
	IDEN,

	// Keywords
	LET,
	FN,
	IF,
	ELIF,
	ELSE,
	FOR,
	IN,
	WHILE,
	RETURN,
	CONTINUE,
	BREAK,
	VOID,
	TRUE,
	FALSE,
	NIL,
	OR,
	CONST,
	DEFER,

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
	STCALL, // instantiate structs

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
		return val == INT || val == FLT || val == CHAR || val == STR || val == IDEN ||
		       val == VOID || val == TRUE || val == FALSE || val == NIL;
	}
	inline bool isLiteral() const
	{
		return val == INT || val == FLT || val == CHAR || val == STR || val == TRUE ||
		       val == FALSE;
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
			val == BNOT_ASSN || val == BXOR_ASSN || val == LSHIFT_ASSN ||
			val == RSHIFT_ASSN);
	}

	inline bool isValid() const { return val != INVALID && val != FEOF; }

	inline const char *cStr() const { return TokStrs[val]; }
	inline String str() const { return TokStrs[val]; }

	const char *getOperCStr() const;
	const char *getUnaryNoCharCStr() const;

	inline bool operator==(Tok other) const { return val == other.val; }

	inline TokType getVal() const { return val; }

	inline void setVal(TokType v) { val = v; }

	inline bool isType(TokType other) const { return val == other; }
};

struct Data
{
	StringRef s;
	int64_t i;
	long double f;

	bool cmp(const Data &other, TokType type) const;
};

class Lexeme
{
	const ModuleLoc *loc;
	Tok tok;
	Data data;

public:
	Lexeme(const ModuleLoc *loc = nullptr);
	explicit Lexeme(const ModuleLoc *loc, TokType type);
	explicit Lexeme(const ModuleLoc *loc, TokType type, StringRef _data);
	explicit Lexeme(const ModuleLoc *loc, TokType type, int64_t _data);
	explicit Lexeme(const ModuleLoc *loc, long double _data);

	String str(int64_t pad = 10) const;

	inline bool operator==(const Lexeme &other) const
	{
		return tok == other.tok && data.cmp(other.data, tok.getVal());
	}
	inline bool operator!=(const Lexeme &other) const { return *this == other ? false : true; }

	inline void setDataStr(StringRef str) { data.s = str; }
	inline void setDataInt(int64_t i) { data.i = i; }
	inline void setDataFlt(long double f) { data.f = f; }

	inline StringRef getDataStr() const { return data.s; }
	inline int64_t getDataInt() const { return data.i; }
	inline const long double &getDataFlt() const { return data.f; }

	inline Tok &getTok() { return tok; }
	inline const Tok &getTok() const { return tok; }
	inline TokType getTokVal() const { return tok.getVal(); }
	inline const ModuleLoc *getLoc() const { return loc; }
};

class Tokenizer
{
	Context &ctx;
	Module *mod;

	ModuleLoc *locAlloc(size_t line, size_t col);
	ModuleLoc loc(size_t line, size_t col);

	StringRef getName(StringRef data, size_t &i);
	TokType classifyStr(StringRef str);
	StringRef getNum(StringRef data, size_t &i, size_t &line, size_t &line_start,
			 TokType &num_type, int &base);
	bool getConstStr(StringRef data, char &quote_type, size_t &i, size_t &line,
			 size_t &line_start, String &buf);
	TokType getOperator(StringRef data, size_t &i, size_t line, size_t line_start);
	void removeBackSlash(String &s);

public:
	Tokenizer(Context &ctx, Module *m);
	bool tokenize(StringRef data, Vector<Lexeme> &toks);
};

} // namespace fer::lex