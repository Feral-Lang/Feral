/*
	MIT License

	Copyright (c) 2020 Feral Language repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#ifndef COMPILER_PARSER_PARSE_HELPER_HPP
#define COMPILER_PARSER_PARSE_HELPER_HPP

#include "../../VM/SrcFile.hpp"
#include "../Lex.hpp"

class phelper_t
{
	lex::toks_t &m_toks; // requires modification at parsing stage
	lex::tok_t m_invalid, m_eof;
	size_t m_idx;

public:
	phelper_t(lex::toks_t &toks, const size_t begin = 0);

	const lex::tok_t *peak(const int offset = 0) const;
	TokType peakt(const int offset = 0) const;

	const lex::tok_t *next();
	TokType nextt();

	const lex::tok_t *prev();
	TokType prevt();

	inline void sett(const TokType type)
	{
		if(m_idx < m_toks.size()) m_toks[m_idx].type = type;
	}

	inline bool accept(const TokType type) const { return peakt() == type; }
	inline bool accept(const TokType t1, const TokType t2) const
	{
		const TokType t = peakt();
		return t == t1 || t == t2;
	}
	inline bool accept(const TokType t1, const TokType t2, const TokType t3) const
	{
		const TokType t = peakt();
		return t == t1 || t == t2 || t == t3;
	}
	inline bool acceptd() const { return lex::tok_type_is_data(peakt()); }

	inline bool valid() { return !accept(TOK_INVALID, TOK_EOF); }

	const lex::tok_t *at(const size_t &idx) const;

	inline bool has_next() const { return m_idx + 1 < m_toks.size(); }

	inline size_t idx() const { return m_idx; }
	inline void set_idx(const size_t &idx) { m_idx = idx; }
};

#endif // COMPILER_PARSER_PARSE_HELPER_HPP
