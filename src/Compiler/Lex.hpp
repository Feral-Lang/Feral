/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef COMPILER_LEX_HPP
#define COMPILER_LEX_HPP

#include <vector>
#include <string>

#include "../Common/Errors.hpp"
#include "../VM/SrcFile.hpp"

/**
 * \brief All valid lexical tokens in the language
 */
enum TokType
{
	TOK_INT,
	TOK_FLT,

	TOK_STR,
	TOK_IDEN,

	//Keywords
	TOK_GLOBAL,
	TOK_LET,
	TOK_DEFER,
	TOK_FN,
	TOK_IF,
	TOK_ELIF,
	TOK_ELSE,
	TOK_FOR,
	TOK_IN,
	TOK_WHILE,
	TOK_RETURN,
	TOK_CONTINUE,
	TOK_BREAK,
	TOK_TRUE,
	TOK_FALSE,
	TOK_NIL,

	// Operators
	TOK_ASSN,
	// Arithmetic
	TOK_ADD,
	TOK_SUB,
	TOK_MUL,
	TOK_DIV,
	TOK_MOD,
	TOK_ADD_ASSN,
	TOK_SUB_ASSN,
	TOK_MUL_ASSN,
	TOK_DIV_ASSN,
	TOK_MOD_ASSN,
	TOK_POW, // **
	// Post/Pre Inc/Dec
	TOK_XINC,
	TOK_INCX,
	TOK_XDEC,
	TOK_DECX,
	// Unary
	TOK_UADD,
	TOK_USUB,
	// Logic
	TOK_AND,
	TOK_OR,
	TOK_NOT,
	// Comparison
	TOK_EQ,
	TOK_LT,
	TOK_GT,
	TOK_LE,
	TOK_GE,
	TOK_NE,
	// Bitwise
	TOK_BAND,
	TOK_BOR,
	TOK_BNOT,
	TOK_BXOR,
	TOK_BAND_ASSN,
	TOK_BOR_ASSN,
	TOK_BNOT_ASSN,
	TOK_BXOR_ASSN,
	// Others
	TOK_LSHIFT,
	TOK_RSHIFT,
	TOK_LSHIFT_ASSN,
	TOK_RSHIFT_ASSN,

	// Dummy
	TOK_OPER_FN,
	TOK_OPER_SUBS,

	// Varargs
	TOK_TDOT,

	// Separators
	TOK_DOT,
	TOK_SCOPE,
	TOK_COMMA,
	TOK_AT,
	TOK_SPC,
	TOK_TAB,
	TOK_NEWL,
	TOK_COLS, // Semi colon
	// Parenthesis, Braces, Brackets
	TOK_LPAREN,
	TOK_RPAREN,
	TOK_LBRACE,
	TOK_RBRACE,
	TOK_LBRACK,
	TOK_RBRACK,

	TOK_EOF,
	TOK_INVALID,

	_TOK_LAST,
};

/**
 * \brief String value of each of the lexical tokens
 */
extern const char * TokStrs[ _TOK_LAST ];

namespace lex
{

/**
 * \brief Describes line number, column number,
 * 	  token type, and string data of a token
 */
struct tok_t
{
	size_t pos;
	TokType type;
	std::string data;

	tok_t( size_t _pos, int _type, std::string _data ) :
	       pos( _pos ), type( ( TokType )_type ), data( _data ) {}
};

/**
 * \brief A list of tokens
 */
typedef std::vector< tok_t > toks_t;

/**
 * \brief Main tokenizing function which is called for generating
 *	  tokens from source code and store them as a vector of tok_t (toks_t)
 *
 * \param src_file Source file object of the source code
 * \param toks Vector where tokens should be stored
 * \param prefix_idx Optionally, position from where the content should be read
 * \return Errors Status of tokenizing operation
 */
Errors tokenize( const srcfile_t & src_file, toks_t & toks, const size_t prefix_idx = 0 );

/**
 * \brief Check if the given type (int) is a variable data
 *
 * A 'variable data' consists of ints, floats, const strings, and identifiers
 *
 * \param type From enum TokType
 * \return true If the type is one of variable data tokens, false if it isn't
 */
inline bool tok_type_is_data( const int type )
{
	return type == TOK_INT || type == TOK_FLT ||
	       type == TOK_STR || type == TOK_IDEN ||
	       type == TOK_TRUE || type == TOK_FALSE ||
	       type == TOK_NIL || type == TOK_TDOT;
}

/**
 * \brief Check if the given type (int) is an operator
 *
 * \param type - From enum TokType
 * \return true If the type is one of possible operators, false if it isn't
 */
inline bool tok_type_is_oper( const int type )
{
	return type >= TOK_ASSN && type <= TOK_RBRACK;
}

/**
 * \brief Check if the given type (int) is an assignment operator
 *
 * \param type - From enum TokType
 * \return true If the type is one of possible assignment operators, false if it isn't
 */
inline bool tok_type_is_assign( const int type )
{
	return ( type == TOK_ASSN ||
		 type == TOK_ADD_ASSN ||
		 type == TOK_SUB_ASSN ||
		 type == TOK_MUL_ASSN ||
		 type == TOK_DIV_ASSN ||
		 type == TOK_MOD_ASSN ||
		 type == TOK_BAND_ASSN ||
		 type == TOK_BOR_ASSN ||
		 type == TOK_BNOT_ASSN ||
		 type == TOK_BXOR_ASSN ||
		 type == TOK_LSHIFT_ASSN ||
		 type == TOK_RSHIFT_ASSN
	);
}

/**
 * \brief Check if the given token pointer's type is a variable data
 *
 * A 'variable data' consists of ints, floats, const strings, and identifiers
 *
 * This function calls the equivalent tok_type_is_data function
 *
 * \param tok Pointer to a tok_t
 * \return true If the type is one of variable data tokens, false if it isn't
 */
inline bool tok_is_data( const tok_t * tok )
{
	return tok_type_is_data( tok->type );
}

/**
 * \brief Check if the given token pointer's type is an operator
 *
 * This function calls the equivalent tok_type_is_oper function
 *
 * \param tok Pointer to a tok_t
 * \return true If the type is one of operator tokens, false if it isn't
 */
inline bool tok_is_oper( const tok_t * tok )
{
	return tok_type_is_oper( tok->type );
}

/**
 * \brief Check if the given token pointer's type is an assignment operator
 *
 * This function calls the equivalent tok_type_is_assign function
 *
 * \param tok Pointer to a tok_t
 * \return true If the type is one of assignment operator tokens, false if it isn't
 */
inline bool tok_is_assign( const tok_t * tok )
{
	return tok_type_is_assign( tok->type );
}

}

#endif // COMPILER_LEX_HPP
