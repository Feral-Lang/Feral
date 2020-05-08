/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <cstdarg>

#include "ParseHelper.hpp"

phelper_t::phelper_t( lex::toks_t & toks, const size_t begin )
	: m_toks( toks ), m_invalid( 0, TOK_INVALID, "" ),
	  m_eof( toks.size() > 0 ? toks.back().pos : 0, TOK_EOF, "" ), m_idx( begin ),
	  m_skip_expr_cols( false ) {}

const lex::tok_t * phelper_t::peak( const int offset ) const
{
	if( offset < 0 && m_idx < ( -offset ) ) return & m_eof;
	if( m_idx + offset >= m_toks.size() ) return & m_eof;
	return & m_toks[ m_idx + offset ];
}

TokType phelper_t::peakt( const int offset ) const
{
	if( offset < 0 && m_idx < ( -offset ) ) return m_eof.type;
	if( m_idx + offset >= m_toks.size() ) return m_eof.type;
	return m_toks[ m_idx + offset ].type;
}

const lex::tok_t * phelper_t::next()
{
	++m_idx;
	if( m_idx >= m_toks.size() ) return & m_eof;
	return & m_toks[ m_idx ];
}

TokType phelper_t::nextt()
{
	++m_idx;
	if( m_idx >= m_toks.size() ) return m_eof.type;
	return m_toks[ m_idx ].type;
}

const lex::tok_t * phelper_t::prev()
{
	if( m_idx == 0 ) return & m_invalid;
	--m_idx;
	return & m_toks[ m_idx ];
}

TokType phelper_t::prevt()
{
	if( m_idx == 0 ) return m_invalid.type;
	--m_idx;
	return m_toks[ m_idx ].type;
}

const lex::tok_t * phelper_t::at( const size_t & idx ) const
{
	if( idx >= m_toks.size() ) return & m_invalid;
	return & m_toks[ idx ];
}