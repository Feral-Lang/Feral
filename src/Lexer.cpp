#include "Lexer.hpp"

#include <charconv>

#include "Error.hpp"

namespace fer::lex
{

const char *TokStrs[_LAST] = {
"INT",
"FLT",
"STR",
"IDEN",

// Keywords
"let",
"fn",
"if",
"elif",
"else",
"for",
"in",
"while",
"return",
"continue",
"break",
"void",
"true",
"false",
"nil",
"or",
"const",
"defer",
"inline",

// Operators
"=",
// Arithmetic
"+",
"-",
"*",
"/",
"%",
"+=",
"-=",
"*=",
"/=",
"%=",
"**", // power
"//", // root
// Post/Pre Inc/Dec
"x++",
"++x",
"x--",
"--x",
// Unary (used by parser (in Expression.cpp))
"u+",
"u-",
"u&",
"u*",
// Logic
"&&",
"||",
"!",
// Comparison
"==",
"<",
">",
"<=",
">=",
"!=",
// Bitwise
"&",
"|",
"~",
"^",
"&=",
"|=",
"~=",
"^=",
// Others
"<<",
">>",
"<<=",
">>=",

"[]",
"()",
"{}",

// Varargs
"...x",
"x...",

// Separators
".",
"?",
":",
",",
"@",
"SPC",
"TAB",
"NEWL",
";",
"->",
// Parenthesis, Braces, Brackets
"(",
")",
"{",
"}",
"[",
"]",

"<FEOF>",
"<INVALID>",
};

Tok::Tok(int tok) : val((TokType)tok) {}

const char *Tok::getUnaryNoCharCStr() const
{
	switch(val) {
	case XINC:
	case INCX: return "++";
	case XDEC:
	case DECX: return "--";
	case UADD: return "+";
	case USUB: return "-";
	case UAND: return "&";
	case UMUL: return "*";
	case LNOT: return "!";
	case BNOT: return "~";
	default: break;
	}
	return "";
}

const char *Tok::getOperCStr() const
{
	switch(val) {
	case ASSN: return "__assn__";
	// Arithmetic
	case ADD: return "__add__";
	case SUB: return "__sub__";
	case MUL: return "__mul__";
	case DIV: return "__div__";
	case MOD: return "__mod__";
	case ADD_ASSN: return "__add_assn__";
	case SUB_ASSN: return "__sub_assn__";
	case MUL_ASSN: return "__mul_assn__";
	case DIV_ASSN: return "__div_assn__";
	case MOD_ASSN: return "__mod_assn__";
	// Post/Pre Inc/Dec
	case XINC: return "__xinc__";
	case INCX: return "__incx__";
	case XDEC: return "__xdec__";
	case DECX: return "__decx__";
	// Unary
	case UADD: return "__uadd__";
	case USUB: return "__usub__";
	// Logic
	case LAND: return "__logand__";
	case LOR: return "__logor__";
	case LNOT: return "__lognot__";
	// Comparison
	case EQ: return "__eq__";
	case LT: return "__lt__";
	case GT: return "__gt__";
	case LE: return "__le__";
	case GE: return "__ge__";
	case NE: return "__ne__";
	// Bitwise
	case BAND: return "__band__";
	case BOR: return "__bor__";
	case BNOT: return "__bnot__";
	case BXOR: return "__bxor__";
	case BAND_ASSN: return "__band_assn__";
	case BOR_ASSN: return "__bor_assn__";
	case BNOT_ASSN: return "__bnot_assn__";
	case BXOR_ASSN: return "__bxor_assn__";
	// Others
	case LSHIFT: return "__lshift__";
	case RSHIFT: return "__rshift__";
	case LSHIFT_ASSN: return "__lshift_assn__";
	case RSHIFT_ASSN: return "__rshift_assn__";
	case SUBS: return "__subscr__";
	default: break;
	}
	return "";
}

Lexeme::Lexeme(const ModuleLoc *loc) : loc(loc), tok(INVALID) {}
Lexeme::Lexeme(const ModuleLoc *loc, TokType type) : loc(loc), tok(type) {}
Lexeme::Lexeme(const ModuleLoc *loc, TokType type, String &&_data)
	: loc(loc), tok(type), data(std::move(_data))
{}
Lexeme::Lexeme(const ModuleLoc *loc, TokType type, StringRef _data)
	: loc(loc), tok(type), data(String(_data))
{}
Lexeme::Lexeme(const ModuleLoc *loc, int64_t _data) : loc(loc), tok(INT), data(_data) {}
Lexeme::Lexeme(const ModuleLoc *loc, long double _data) : loc(loc), tok(FLT), data(_data) {}

bool Lexeme::cmpData(const Lexeme &other, const TokType type) const
{
	switch(type) {
	case STR: // fallthrough
	case IDEN: return std::get<String>(data) == std::get<String>(other.data);
	case INT: return std::get<int64_t>(data) == std::get<int64_t>(other.data);
	case FLT: return std::get<long double>(data) == std::get<long double>(other.data);
	default: return false;
	}
	return false;
}

String Lexeme::str(int64_t pad) const
{
	String res;
	int64_t len;
	res += tok.cStr();
	len = res.size();
	for(int64_t i = 0; i < pad - len; ++i) res += " ";
	if(pad == 0) res += " ";
	len = res.size();
	if(loc) res += "[" + loc->getLocStr() + "]";
	if(!tok.isData()) return res;
	len = res.size() - len;
	for(int64_t i = 0; i < pad - len; ++i) res += " ";
	if(pad == 0) res += " ";
	if(tok.getVal() == STR || tok.getVal() == IDEN) {
		res += getDataStr();
	} else if(tok.getVal() == INT) {
		res += std::to_string(getDataInt());
	} else if(tok.getVal() == FLT) {
		res += std::to_string(getDataFlt());
	}
	return res;
}

#define CURR (data[i])
#define NEXT (i + 1 < len ? data[i + 1] : 0)
#define PREV (len > 0 && i > 0 ? data[i - 1] : 0)
#define SET_OP_TYPE_BRK(type) \
	op_type = type;       \
	break

Tokenizer::Tokenizer(Context &ctx, Module *m) : ctx(ctx), mod(m) {}

ModuleLoc *Tokenizer::locAlloc(size_t line, size_t col)
{
	return ctx.allocModuleLoc(mod, line, col);
}
ModuleLoc Tokenizer::loc(size_t line, size_t col) { return ModuleLoc(mod, line, col); }

bool Tokenizer::tokenize(String &data, Vector<Lexeme> &toks)
{
	int comment_block = 0; // int to handle nested comment blocks
	bool comment_line = false;

	size_t len	  = data.size();
	size_t i	  = 0;
	size_t line	  = 0;
	size_t line_start = 0;
	while(i < len) {
		if(CURR == '\n') {
			++line;
			line_start = i + 1;
		}
		if(comment_line) {
			if(CURR == '\n') comment_line = false;
			++i;
			continue;
		}
		if(isspace(CURR)) {
			++i;
			continue;
		}
		if(CURR == '*' && NEXT == '/') {
			if(!comment_block) {
				err::out(loc(line, i - line_start),
					 "encountered multi line comment "
					 "terminator '*/' in non comment block");
				return false;
			}
			i += 2;
			--comment_block;
			continue;
		}
		if(CURR == '/' && NEXT == '*') {
			i += 2;
			++comment_block;
			continue;
		}
		if(comment_block) {
			++i;
			continue;
		}
		if(CURR == '#') {
			comment_line = true;
			++i;
			continue;
		}

		// strings
		if((CURR == '.' && (isalpha(NEXT) || NEXT == '_') && !isalnum(PREV) &&
		    PREV != '_' && PREV != ')' && PREV != ']' && PREV != '\'' && PREV != '"') ||
		   isalpha(CURR) || CURR == '_')
		{
			String tmpstr; // used for __SRC_PATH__ and __SRC_DIR__
			StringRef str = getName(data, i);
			// check if string is a keyword
			TokType str_class = classifyStr(str);
			size_t strsz	  = str.size();
			if(!str.empty() && str[0] == '.') str = str.substr(1);
			if(str == "__SRC_PATH__") {
				// toRawString() because in codegen, all strings are passed through
				// fromRawString()
				tmpstr	  = toRawString(mod->getPath());
				str	  = tmpstr;
				str_class = STR;
			} else if(str == "__SRC_DIR__") {
				tmpstr	  = toRawString(mod->getDir());
				str	  = tmpstr;
				str_class = STR;
			}
			if(str_class == STR || str_class == IDEN) {
				// place either the data itself (type = STR, IDEN)
				toks.emplace_back(locAlloc(line, i - line_start - strsz), str_class,
						  str);
			} else {
				// or the type
				toks.emplace_back(locAlloc(line, i - line_start - strsz),
						  str_class);
			}
			continue;
		}

		// numbers
		if(isdigit(CURR)) {
			TokType num_type = INT;
			int base	 = 10;
			StringRef num	 = getNum(data, i, line, line_start, num_type, base);
			if(num.empty()) return false;
			if(num_type == FLT) {
				// FIXME: from_chars() does not work with LLVM's libc++
#if defined(_LIBCPP_VERSION)
				String numtmp(num);
				char *end	   = NULL;
				long double fltval = std::strtold(numtmp.c_str(), &end);
#else
				long double fltval;
				std::from_chars(num.data(), num.data() + num.size(), fltval);
#endif
				toks.emplace_back(locAlloc(line, i - line_start - num.size()),
						  fltval);
				continue;
			}
			int64_t intval;
			if(num.size() > 2 && base != 10) {
				// base of 8: starts with 0 => 0755
				// everything else: starts with 0 and letter
				num = num.substr(base == 8 ? 1 : 2);
			}
			std::from_chars(num.data(), num.data() + num.size(), intval, base);
			toks.emplace_back(locAlloc(line, i - line_start - num.size()), intval);
			continue;
		}

		// const strings
		if(CURR == '\"' || CURR == '\'' || CURR == '`') {
			StringRef buf;
			size_t startloc = i + 1;
			char quote_type = 0;
			if(!getConstStr(data, quote_type, len, i, line, line_start, buf))
				return false;
			toks.emplace_back(locAlloc(line, startloc - line_start), STR, buf);
			continue;
		}

		// operators
		size_t begin	= i;
		TokType op_type = getOperator(data, i, line, line_start);
		if(op_type == INVALID) return false;
		toks.emplace_back(locAlloc(line, begin - line_start), op_type);
	}
	return true;
}

StringRef Tokenizer::getName(StringRef data, size_t &i)
{
	size_t len   = data.size();
	size_t start = i++; // we know first char is valid, duh
	while(i < len) {
		if(!isalnum(CURR) && CURR != '_') break;
		++i;
	}
	if(i < len && CURR == '?') ++i;

	return StringRef(&data[start], i - start);
}

TokType Tokenizer::classifyStr(StringRef str)
{
	if(str == TokStrs[LET]) return LET;
	if(str == TokStrs[FN]) return FN;
	if(str == TokStrs[IF]) return IF;
	if(str == TokStrs[ELIF]) return ELIF;
	if(str == TokStrs[ELSE]) return ELSE;
	if(str == TokStrs[FOR]) return FOR;
	if(str == TokStrs[IN]) return IN;
	if(str == TokStrs[WHILE]) return WHILE;
	if(str == TokStrs[RETURN]) return RETURN;
	if(str == TokStrs[CONTINUE]) return CONTINUE;
	if(str == TokStrs[BREAK]) return BREAK;
	if(str == TokStrs[VOID]) return VOID;
	if(str == TokStrs[TRUE]) return TRUE;
	if(str == TokStrs[FALSE]) return FALSE;
	if(str == TokStrs[NIL]) return NIL;
	if(str == TokStrs[OR]) return OR;
	if(str == TokStrs[CONST]) return CONST;
	if(str == TokStrs[DEFER]) return DEFER;
	if(str == TokStrs[INLINE]) return INLINE;

	// if string begins with dot, it's an atom (str), otherwise an identifier
	return str[0] == '.' ? STR : IDEN;
}

StringRef Tokenizer::getNum(StringRef data, size_t &i, size_t &line, size_t &line_start,
			    TokType &num_type, int &base)
{
	size_t len	      = data.size();
	size_t first_digit_at = i;

	int dot_loc = -1;
	base	    = 10;

	bool read_base = false;

	while(i < len) {
		const char c	= CURR;
		const char next = NEXT;
		switch(c) {
		case 'x':
		case 'X': {
			if(read_base) {
				base	  = 16;
				read_base = false;
				break;
			}
			goto fail;
		}
		case 'f':
		case 'F':
		case 'e':
		case 'E':
		case 'd':
		case 'D':
		case 'c':
		case 'C':
		case 'b':
		case 'B':
		case 'a':
		case 'A': {
			if(base >= 16) break;
			goto fail;
		}
		case '9':
		case '8':
		case '7':
			if(base >= 8) break;
			goto fail;
		case '6':
		case '5':
		case '4':
		case '3':
		case '2':
			if(base > 2) break;
			goto fail;
		case '1': break;
		case '0': {
			if(i == first_digit_at) {
				read_base = true;
				base	  = 8;
			}
			break;
		}
		case '.':
			if(!read_base && base != 10) {
				err::out(loc(line, first_digit_at - line_start),
					 "encountered dot (.) character when base is not 10 (",
					 base, ") ");
				return "";
			} else if(dot_loc == -1) {
				if(next >= '0' && next <= '9') {
					dot_loc	 = i;
					num_type = FLT;
				} else {
					goto end;
				}
			} else {
				err::out(loc(line, first_digit_at - line_start),
					 "encountered dot (.) character when the "
					 "number being retrieved (from column ",
					 first_digit_at + 1, ") already had one");
				return "";
			}
			read_base = false;
			base	  = 10;
			break;
		default:
		fail:
			if(isalnum(c)) {
				err::out(loc(line, first_digit_at - line_start),
					 "encountered invalid character '", c,
					 "' while retrieving a number of base ", base);
				return "";
			} else {
				goto end;
			}
		}
		if(first_digit_at != i || c != '0') read_base = false;
		++i;
	}
end:
	return StringRef(&data[first_digit_at], i - first_digit_at);
}

bool Tokenizer::getConstStr(String &data, char &quote_type, size_t &len, size_t &i, size_t &line,
			    size_t &line_start, StringRef &buf)
{
	quote_type		    = CURR;
	int starting_at		    = i + 1;
	size_t continuous_backslash = 0;
	// omit beginning quote
	++i;
	while(i < len) {
		if(CURR == '\n') {
			++line;
			line_start = i + 1;
		}
		if(CURR == '\\') {
			++continuous_backslash;
			++i;
			continue;
		}
		if(CURR == quote_type && continuous_backslash % 2 == 0) break;
		++i;
		continuous_backslash = 0;
	}
	if(CURR != quote_type) {
		err::out(loc(line, starting_at - line_start), "no matching quote for '", quote_type,
			 "' found");
		return false;
	}
	buf = StringRef(&data[starting_at], i - starting_at);
	// omit ending quote
	++i;
	return true;
}

TokType Tokenizer::getOperator(StringRef data, size_t &i, size_t line, size_t line_start)
{
	size_t len	   = data.size();
	TokType op_type	   = INVALID;
	size_t starting_at = i;
	switch(CURR) {
	case '+':
		if(i < len - 1) {
			if(NEXT == '=') {
				++i;
				SET_OP_TYPE_BRK(ADD_ASSN);
			}
			if(NEXT == '+') {
				++i;
				SET_OP_TYPE_BRK(XINC);
			}
		}
		SET_OP_TYPE_BRK(ADD);
	case '-':
		if(i < len - 1) {
			if(NEXT == '=') {
				++i;
				SET_OP_TYPE_BRK(SUB_ASSN);
			}
			if(NEXT == '-') {
				++i;
				SET_OP_TYPE_BRK(XDEC);
			}
			if(NEXT == '>') {
				++i;
				SET_OP_TYPE_BRK(ARROW);
			}
		}
		SET_OP_TYPE_BRK(SUB);
	case '*':
		if(i < len - 1) {
			if(NEXT == '=' || NEXT == '*') {
				++i;
				if(CURR == '=') op_type = MUL_ASSN;
				else if(CURR == '*') op_type = POWER;
				break;
			}
		}
		SET_OP_TYPE_BRK(MUL);
	case '/':
		if(i < len - 1) {
			if(NEXT == '=' || NEXT == '/') {
				++i;
				if(CURR == '=') op_type = DIV_ASSN;
				else if(CURR == '/') op_type = ROOT;
				break;
			}
		}
		SET_OP_TYPE_BRK(DIV);
	case '%':
		if(i < len - 1) {
			if(NEXT == '=') {
				++i;
				SET_OP_TYPE_BRK(MOD_ASSN);
			}
		}
		SET_OP_TYPE_BRK(MOD);
	case '&':
		if(i < len - 1) {
			if(NEXT == '&' || NEXT == '=') {
				++i;
				if(CURR == '&') op_type = LAND;
				else if(CURR == '=') op_type = BAND_ASSN;
				break;
			}
		}
		SET_OP_TYPE_BRK(BAND);
	case '|':
		if(i < len - 1) {
			if(NEXT == '|' || NEXT == '=') {
				++i;
				if(CURR == '|') op_type = LOR;
				else if(CURR == '=') op_type = BOR_ASSN;
				break;
			}
		}
		SET_OP_TYPE_BRK(BOR);
	case '~':
		if(i < len - 1) {
			if(NEXT == '=') {
				++i;
				SET_OP_TYPE_BRK(BNOT_ASSN);
			}
		}
		SET_OP_TYPE_BRK(BNOT);
	case '=':
		if(i < len - 1) {
			if(NEXT == '=') {
				++i;
				SET_OP_TYPE_BRK(EQ);
			}
		}
		SET_OP_TYPE_BRK(ASSN);
	case '<':
		if(i < len - 1) {
			if(NEXT == '=' || NEXT == '<') {
				++i;
				if(CURR == '=') op_type = LE;
				else if(CURR == '<') {
					if(i < len - 1) {
						if(NEXT == '=') {
							++i;
							SET_OP_TYPE_BRK(LSHIFT_ASSN);
						}
					}
					op_type = LSHIFT;
				}
				break;
			}
		}
		SET_OP_TYPE_BRK(LT);
	case '>':
		if(i < len - 1) {
			if(NEXT == '=' || NEXT == '>') {
				++i;
				if(CURR == '=') op_type = GE;
				else if(CURR == '>') {
					if(i < len - 1) {
						if(NEXT == '=') {
							++i;
							SET_OP_TYPE_BRK(RSHIFT_ASSN);
						}
					}
					op_type = RSHIFT;
				}
				break;
			}
		}
		SET_OP_TYPE_BRK(GT);
	case '!':
		if(i < len - 1) {
			if(NEXT == '=') {
				++i;
				SET_OP_TYPE_BRK(NE);
			}
		}
		SET_OP_TYPE_BRK(LNOT);
	case '^':
		if(i < len - 1) {
			if(NEXT == '=') {
				++i;
				SET_OP_TYPE_BRK(BXOR_ASSN);
			}
		}
		SET_OP_TYPE_BRK(BXOR);
	case ' ': SET_OP_TYPE_BRK(SPC);
	case '\t': SET_OP_TYPE_BRK(TAB);
	case '\n': SET_OP_TYPE_BRK(NEWL);
	case '.':
		if(i < len - 1 && NEXT == '.') {
			++i;
			if(i < len - 1 && NEXT == '.') {
				++i;
				SET_OP_TYPE_BRK(PreVA);
			}
		}
		SET_OP_TYPE_BRK(DOT);
	case '?': SET_OP_TYPE_BRK(QUEST);
	case ':': SET_OP_TYPE_BRK(COL);
	case ',': SET_OP_TYPE_BRK(COMMA);
	case ';': SET_OP_TYPE_BRK(COLS);
	case '@': SET_OP_TYPE_BRK(AT);
	case '(': SET_OP_TYPE_BRK(LPAREN);
	case '[': SET_OP_TYPE_BRK(LBRACK);
	case '{': SET_OP_TYPE_BRK(LBRACE);
	case ')': SET_OP_TYPE_BRK(RPAREN);
	case ']': SET_OP_TYPE_BRK(RBRACK);
	case '}': SET_OP_TYPE_BRK(RBRACE);
	default:
		err::out(loc(line, starting_at - line_start), "unknown operator '", CURR,
			 "' found");
		op_type = INVALID;
	}

	++i;
	return op_type;
}

} // namespace fer::lex