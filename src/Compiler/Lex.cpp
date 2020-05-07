/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Lex.hpp"

const char * TokStrs[ _TOK_LAST ] = {
	"INT",
	"FLT",

	"STR",
	"IDEN",

	//Keywords
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
	"true",
	"false",
	"nil",

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
	// Post/Pre Inc/Dec
	"x++",
	"++x",
	"x--",
	"--x",
	// Unary (used by parser (in Expression.cpp))
	"u+",
	"u-",
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

	// Functions
	"()", // func
	".()", // member func

	// Dummy
	".x()", // attribute - member func

	// Subscript
	"[]",

	// Varargs
	"...",

	// Separators
	".",
	":",
	",",
	"@",
	"SPC",
	"TAB",
	"NEWL",
	";",
	// Parenthesis, Braces, Brackets
	"(",
	")",
	"{",
	"}",
	"[",
	"]",

	"<EOF>",
	"<INVALID>",
};

#define CURR( src ) ( src[ i ] )
#define NEXT( src ) ( i + 1 < src_len ? src[ i + 1 ] : 0 )
#define PREV( src ) ( src_len > 0 && i > 0 ? src[ i - 1 ] : 0 )
#define SET_OP_TYPE_BRK( type ) op_type = type; break

#define SRC_FAIL( ... ) src_file.fail( i, __VA_ARGS__ )


static std::string get_name( const srcfile_t & src_file, size_t & i );
static int classify_str( const std::string & str );
static std::string get_num( const srcfile_t & src_file, size_t & i, int & num_type );
static Errors get_const_str( const srcfile_t & src_file, size_t & i, std::string & buf );
static int get_operator( const srcfile_t & src_file, size_t & i );
static inline bool is_valid_num_char( const char c );
static void remove_back_slash( std::string & s );

namespace lex
{

Errors tokenize( const srcfile_t & src_file, lex::toks_t & toks, const size_t & begin_idx, const size_t & end_idx )
{
	const std::string & src = src_file.data();
	if( src.empty() ) return E_OK;

	Errors err = E_OK;
	size_t src_len = src.size();

	bool comment_block = false;
	bool comment_line = false;

	// tokenize the input
	size_t i = begin_idx;
	while( i < end_idx ) {
		if( comment_line && CURR( src ) == '\n' ) {
			comment_line = false;
			++i;
			continue;
		}
		if( isspace( src[ i ] ) ) { ++i; continue; }

		if( !comment_line && CURR( src ) == '*' && NEXT( src ) == '/' ) {
			if( !comment_block ) {
				SRC_FAIL( "encountered multi line comment terminator '*/' "
					  "in non commented block" );
				err = E_LEX_FAIL;
				break;
			}
			i += 2;
			comment_block = false;
			continue;
		}

		if( !comment_line && CURR( src ) == '/' && NEXT( src ) == '*' ) {
			i += 2;
			comment_block = true;
			continue;
		}

		if( comment_block ) { ++i; continue; }

		if( comment_line ) {
			if( CURR( src ) == '\n' ) {
				comment_line = false;
			}
			++i;
			continue;
		}
		if( CURR( src ) == '#' ) { comment_line = true; ++i; continue; }

		// strings
		if( ( CURR( src ) == '.' && ( isalpha( NEXT( src ) ) || NEXT( src ) == '_' ) && !isalnum( PREV( src ) ) && PREV( src ) != '_' &&
		      PREV( src ) != ')' && PREV( src ) != ']' && PREV( src ) != '\'' && PREV( src ) != '"' ) ||
		    isalpha( CURR( src ) ) || CURR( src ) == '_' ) {
			std::string str = get_name( src_file, i );
			// check if string is a keyword
			int str_class = classify_str( str );
			if( str == "__SRC_DIR__" || str == "__SRC_PATH__" ) {
				if( str == "__SRC_DIR__" ) str = src_file.dir();
				if( str == "__SRC_PATH__" ) str = src_file.path();
				str_class = TOK_STR;
			}
			if( str[ 0 ] == '.' ) str.erase( str.begin() );
			toks.emplace_back( i - str.size(), str_class, str );
			continue;
		}

		// numbers
		if( isdigit( CURR( src ) ) ) {
			int num_type = TOK_INT;
			std::string num = get_num( src_file, i, num_type );
			if( num.empty() ) {
				err = E_LEX_FAIL;
				break;
			}
			toks.emplace_back( i - num.size(), num_type, num );
			continue;
		}

		// const strings
		if( CURR( src ) == '\"' || CURR( src ) == '\'' || CURR( src ) == '`' ) {
			std::string str;
			Errors res = get_const_str( src_file, i, str );
			if( res != E_OK ) {
				err = res;
				break;
			}
			toks.emplace_back( i - str.size(), TOK_STR, str );
			continue;
		}

		// operators
		int op_type = get_operator( src_file, i );
		if( op_type < 0 ) {
			err = E_LEX_FAIL;
			break;
		}
		if( op_type == TOK_TDOT ) {
			toks.emplace_back( i - 3, op_type, TokStrs[ op_type ] );
		} else {
			toks.emplace_back( i - 1, op_type, "" );
		}
	}

	return err;
}

}

static std::string get_name( const srcfile_t & src_file, size_t & i )
{
	const std::string & src = src_file.data();
	size_t src_len = src.size();
	std::string buf;
	buf.push_back( src[ i++ ] );
	while( i < src_len ) {
		if( !isalnum( CURR( src ) ) && CURR( src ) != '_' ) break;
		buf.push_back( src[ i++ ] );
	}
	if( i < src_len && CURR( src ) == '?' ) buf.push_back( src[ i++ ] );

	return buf;
}

static int classify_str( const std::string & str )
{
	if( str == TokStrs[ TOK_LET ] ) return TOK_LET;
	else if( str == TokStrs[ TOK_FN ] ) return TOK_FN;
	else if( str == TokStrs[ TOK_IF ] ) return TOK_IF;
	else if( str == TokStrs[ TOK_ELIF ] ) return TOK_ELIF;
	else if( str == TokStrs[ TOK_ELSE ] ) return TOK_ELSE;
	else if( str == TokStrs[ TOK_FOR ] ) return TOK_FOR;
	else if( str == TokStrs[ TOK_IN ] ) return TOK_IN;
	else if( str == TokStrs[ TOK_WHILE ] ) return TOK_WHILE;
	else if( str == TokStrs[ TOK_RETURN ] ) return TOK_RETURN;
	else if( str == TokStrs[ TOK_CONTINUE ] ) return TOK_CONTINUE;
	else if( str == TokStrs[ TOK_BREAK ] ) return TOK_BREAK;
	else if( str == TokStrs[ TOK_TRUE ] ) return TOK_TRUE;
	else if( str == TokStrs[ TOK_FALSE ] ) return TOK_FALSE;
	else if( str == TokStrs[ TOK_NIL ] ) return TOK_NIL;

	// if string begins with dot, it's an atom (str), otherwise an identifier
	return str[ 0 ] == '.' ? TOK_STR : TOK_IDEN;
}

static std::string get_num( const srcfile_t & src_file, size_t & i, int & num_type )
{
	const std::string & src = src_file.data();
	size_t src_len = src.size();
	std::string buf;
	int first_digit_at = i;

	bool success = true;
	int dot_encountered = -1;

	while( i < src_len ) {
		const char c = CURR( src );
		const char next = NEXT( src );
		switch( c ) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			break;
		case '.':
			if( dot_encountered == -1 ) {
				if( next >= '0' && next <= '9' ) {
					dot_encountered = i;
					num_type = TOK_FLT;
				} else {
					return buf;
				}
			} else {
				SRC_FAIL( "encountered dot (.) character "
					  "when the number being retrieved (from column %d) "
					  "already had one at column %d",
					  first_digit_at + 1, dot_encountered + 1 );
				success = false;
			}
			break;
		default:
			if( isalnum( c ) ) {
				SRC_FAIL( "encountered invalid character '%c' "
					  "while retrieving a number (from column %d)",
					  c, first_digit_at + 1 );
				success = false;
			} else {
				return buf;
			}

		}
		if( success == false ) {
			return "";
		}
		buf.push_back( c ); ++i;
	}
	return buf;
}

static Errors get_const_str( const srcfile_t & src_file, size_t & i, std::string & buf )
{
	const std::string & src = src_file.data();
	size_t src_len = src.size();
	buf.clear();
	const char quote_type = CURR( src );
	int starting_at = i;
	// omit beginning quote
	++i;
	while( i < src_len ) {
		if( CURR( src ) == quote_type && PREV( src ) != '\\' ) break;
		buf.push_back( src[ i++ ] );
	}
	if( CURR( src ) != quote_type ) {
		i = starting_at;
		SRC_FAIL( "no matching quote for '%c' found", quote_type );
		return E_LEX_FAIL;
	}
	// omit ending quote
	++i;
	remove_back_slash( buf );
	return E_OK;
}

static int get_operator( const srcfile_t & src_file, size_t & i )
{
	const std::string & src = src_file.data();
	size_t src_len = src.size();
	int op_type = -1;
	switch( CURR( src ) ) {
	case '+':
		if( i < src_len - 1 ) {
			if( NEXT( src ) == '=' ) {
				++i;
				SET_OP_TYPE_BRK( TOK_ADD_ASSN );
			}
			if( NEXT( src ) == '+' ) {
				++i;
				SET_OP_TYPE_BRK( TOK_XINC );
			}
		}
		SET_OP_TYPE_BRK( TOK_ADD );
	case '-':
		if( i < src_len - 1 ) {
			if( NEXT( src ) == '=' ) {
				++i;
				SET_OP_TYPE_BRK( TOK_SUB_ASSN );
			}
			if( NEXT( src ) == '-' ) {
				++i;
				SET_OP_TYPE_BRK( TOK_XDEC );
			}
		}
		SET_OP_TYPE_BRK( TOK_SUB );
	case '*':
		if( i < src_len - 1 ) {
			if( NEXT( src ) == '*' || NEXT( src ) == '=' ) {
				++i;
				if( CURR( src ) == '*' ) op_type = TOK_POW;
				else if( CURR( src ) == '=' ) op_type = TOK_MUL_ASSN;
				break;
			}
		}
		SET_OP_TYPE_BRK( TOK_MUL );
	case '/':
		if( i < src_len - 1 ) {
			if( NEXT( src ) == '=' ) {
				++i;
				SET_OP_TYPE_BRK( TOK_DIV_ASSN );
			}
		}
		SET_OP_TYPE_BRK( TOK_DIV );
	case '%':
		if( i < src_len - 1 ) {
			if( NEXT( src ) == '=' ) {
				++i;
				SET_OP_TYPE_BRK( TOK_MOD_ASSN );
			}
		}
		SET_OP_TYPE_BRK( TOK_MOD );
	case '&':
		if( i < src_len - 1 ) {
			if( NEXT( src ) == '&' || NEXT( src ) == '=' ) {
				++i;
				if( CURR( src ) == '&' ) op_type = TOK_AND;
				else if( CURR( src ) == '=' ) op_type = TOK_BAND_ASSN;
				break;
			}
		}
		SET_OP_TYPE_BRK( TOK_BAND );
	case '|':
		if( i < src_len - 1 ) {
			if( NEXT( src ) == '|' || NEXT( src ) == '=' ) {
				++i;
				if( CURR( src ) == '|' ) op_type = TOK_OR;
				else if( CURR( src ) == '=' ) op_type = TOK_BOR_ASSN;
				break;
			}
		}
		SET_OP_TYPE_BRK( TOK_BOR );
	case '~':
		if( i < src_len - 1 ) {
			if( NEXT( src ) == '=' ) {
				++i;
				SET_OP_TYPE_BRK( TOK_BNOT_ASSN );
			}
		}
		SET_OP_TYPE_BRK( TOK_BNOT );
	case '=':
		if( i < src_len - 1 ) {
			if( NEXT( src ) == '=' ) {
				++i;
				SET_OP_TYPE_BRK( TOK_EQ );
			}
		}
		SET_OP_TYPE_BRK( TOK_ASSN );
	case '<':
		if( i < src_len - 1 ) {
			if( NEXT( src ) == '=' || NEXT( src ) == '<' ) {
				++i;
				if( CURR( src ) == '=' ) op_type = TOK_LE;
				else if( CURR( src ) == '<' ) {
					if( i < src_len - 1 ) {
						if( NEXT( src ) == '=' ) {
							++i;
							SET_OP_TYPE_BRK( TOK_LSHIFT_ASSN );
						}
					}
					op_type = TOK_LSHIFT;
				}
				break;
			}
		}
		SET_OP_TYPE_BRK( TOK_LT );
	case '>':
		if( i < src_len - 1 ) {
			if( NEXT( src ) == '=' || NEXT( src ) == '>' ) {
				++i;
				if( CURR( src ) == '=' ) op_type = TOK_GE;
				else if( CURR( src ) == '>' ) {
					if( i < src_len - 1 ) {
						if( NEXT( src ) == '=' ) {
							++i;
							SET_OP_TYPE_BRK( TOK_RSHIFT_ASSN );
						}
					}
					op_type = TOK_RSHIFT;
				}
				break;
			}
		}
		SET_OP_TYPE_BRK( TOK_GT );
	case '!':
		if( i < src_len - 1 ) {
			if( NEXT( src ) == '=' ) {
				++i;
				SET_OP_TYPE_BRK( TOK_NE );
			}
		}
		SET_OP_TYPE_BRK( TOK_NOT );
	case '^':
		if( i < src_len - 1 ) {
			if( NEXT( src ) == '=' ) {
				++i;
				SET_OP_TYPE_BRK( TOK_BXOR_ASSN );
			}
		}
		SET_OP_TYPE_BRK( TOK_BXOR );
	case ' ':
		SET_OP_TYPE_BRK( TOK_SPC );
	case '\t':
		SET_OP_TYPE_BRK( TOK_TAB );
	case '\n':
		SET_OP_TYPE_BRK( TOK_NEWL );
	case '.':
		if( i < src_len - 1 ) {
			if( NEXT( src ) == '.' ) {
				++i;
				if( i < src_len - 1 ) {
					if( NEXT( src ) == '.' ) {
						++i;
						SET_OP_TYPE_BRK( TOK_TDOT );
					}
				}
			}
		}
		SET_OP_TYPE_BRK( TOK_DOT );
	case ':':
		SET_OP_TYPE_BRK( TOK_SCOPE );
	case ',':
		SET_OP_TYPE_BRK( TOK_COMMA );
	case ';':
		SET_OP_TYPE_BRK( TOK_COLS );
	case '@':
		SET_OP_TYPE_BRK( TOK_AT );
	case '(':
		SET_OP_TYPE_BRK( TOK_LPAREN );
	case '[':
		SET_OP_TYPE_BRK( TOK_LBRACK );
	case '{':
		SET_OP_TYPE_BRK( TOK_LBRACE );
	case ')':
		SET_OP_TYPE_BRK( TOK_RPAREN );
	case ']':
		SET_OP_TYPE_BRK( TOK_RBRACK );
	case '}':
		SET_OP_TYPE_BRK( TOK_RBRACE );
	default:
		SRC_FAIL( "unknown operator '%c' found", CURR( src ) );
		op_type = -1;
	}

	++i;
	return op_type;
}

static void remove_back_slash( std::string & s )
{
	for( auto it = s.begin(); it != s.end(); ++it ) {
		if( * it == '\\' ) {
			if( it + 1 >= s.end() ) continue;
			it = s.erase( it );
			if( * it == 'a' ) * it = '\a';
			else if( * it == 'b' ) * it = '\b';
			else if( * it == 'f' ) * it = '\f';
			else if( * it == 'n' ) * it = '\n';
			else if( * it == 'r' ) * it = '\r';
			else if( * it == 't' ) * it = '\t';
			else if( * it == 'v' ) * it = '\v';
		}
	}
}
