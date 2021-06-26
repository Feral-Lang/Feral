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

#include "Compiler/Parser.hpp"

namespace parser
{
Errors parse(phelper_t &ph, lex::toks_t &toks, ptree_t *&ptree, const size_t begin)
{
	if(parse_block(ph, (stmt_base_t *&)ptree, false) != E_OK) goto fail;
	return E_OK;
fail:
	delete ptree;
	return E_PARSE_FAIL;
}

} // namespace parser
