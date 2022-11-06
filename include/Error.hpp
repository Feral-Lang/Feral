#pragma once

#include "Module.hpp"

namespace fer::err
{

void setMaxErrs(size_t max_err);

void outCommon(const ModuleLoc *loc, InitList<StringRef> err, bool is_warn, bool with_loc);

inline void out(InitList<StringRef> err) { outCommon(nullptr, err, false, false); }
inline void out(const ModuleLoc &loc, InitList<StringRef> err)
{
	outCommon(&loc, err, false, true);
}
// equivalent to out(), but for warnings
inline void outw(InitList<StringRef> err) { outCommon(nullptr, err, true, false); }
inline void outw(const ModuleLoc &loc, InitList<StringRef> err)
{
	outCommon(&loc, err, true, true);
}

// extras
void out(Stmt *stmt, InitList<StringRef> err);
void out(const lex::Lexeme &tok, InitList<StringRef> err);
void outw(Stmt *stmt, InitList<StringRef> err);
void outw(const lex::Lexeme &tok, InitList<StringRef> err);

} // namespace fer::err