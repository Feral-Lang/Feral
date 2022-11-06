/*
	MIT License

	Copyright (c) 2022 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "Error.hpp"

#include <iostream>

#include "Module.hpp"
#include "Parser/Stmts.hpp"

namespace fer
{
namespace err
{

static size_t max_errs = 10;
void setMaxErrs(size_t max_err) { max_errs = max_err; }

void out(Stmt *stmt, InitList<StringRef> err) { outCommon(stmt->getLoc(), err, false, true); }
void out(const lex::Lexeme &tok, InitList<StringRef> err)
{
	outCommon(tok.getLoc(), err, false, true);
}
void outw(Stmt *stmt, InitList<StringRef> err) { outCommon(stmt->getLoc(), err, true, true); }
void outw(const lex::Lexeme &tok, InitList<StringRef> err)
{
	outCommon(tok.getLoc(), err, true, true);
}

void outCommon(const ModuleLoc *loc, InitList<StringRef> err, bool is_warn, bool with_loc)
{
	static size_t errcount = 0;

	if(errcount >= max_errs) return;

	// just show the error
	if(!with_loc) {
		std::cout << (is_warn ? "Warning" : "Failure") << ": ";
		for(auto &e : err) std::cout << e;
		std::cout << "\n";
		if(!is_warn) ++errcount;
		if(errcount >= max_errs) std::cout << "Failure: Too many errors encountered\n";
		return;
	}

	Module *mod = loc->getMod();
	size_t line = loc->getLine();
	size_t col  = loc->getCol();

	size_t linectr = 0;
	size_t idx     = 0;
	bool found     = false;

	StringRef data	   = mod->getCode();
	StringRef filename = mod->getPath();

	for(size_t i = 0; i < data.size(); ++i) {
		if(linectr == line) {
			found = true;
			idx   = i;
			break;
		}
		if(data[i] == '\n') {
			++linectr;
			continue;
		}
	}
	StringRef err_line = "<not found>";
	if(found) {
		size_t count = data.find('\n', idx);
		if(count != String::npos) count -= idx;
		err_line = data.substr(idx, count);
	}

	size_t tab_count = 0;
	for(auto &c : err_line) {
		if(c == '\t') ++tab_count;
	}
	String spacing_caret(col, ' ');
	while(tab_count--) {
		spacing_caret.pop_back();
		spacing_caret.insert(spacing_caret.begin(), '\t');
	}

	std::cout << filename << " (" << line + 1 << ":" << col + 1 << "): ";
	std::cout << (is_warn ? "Warning" : "Failure") << ": ";
	for(auto &e : err) std::cout << e;
	std::cout << "\n";
	std::cout << err_line << "\n";
	std::cout << spacing_caret << "^\n";

	if(!is_warn) ++errcount;
	if(errcount >= max_errs) std::cout << "Failure: Too many errors encountered\n";
}

} // namespace err
} // namespace fer