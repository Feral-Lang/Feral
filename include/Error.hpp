#pragma once

#include "Module.hpp"

namespace fer::err
{

void setMaxErrs(size_t max_err);

void outCommon(const ModuleLoc *loc, InitList<StringRef> err, bool is_warn, bool with_loc);

inline void out(const ModuleLoc *loc, InitList<StringRef> err) { outCommon(loc, err, false, loc); }
inline void out(ModuleLoc &&loc, InitList<StringRef> err) { outCommon(&loc, err, false, true); }
inline void out(InitList<StringRef> err) { outCommon(nullptr, err, false, false); }

// equivalent to out(), but for warnings
inline void outw(const ModuleLoc *loc, InitList<StringRef> err) { outCommon(loc, err, true, loc); }
inline void outw(ModuleLoc &&loc, InitList<StringRef> err) { outCommon(&loc, err, true, true); }
inline void outw(InitList<StringRef> err) { outCommon(nullptr, err, true, false); }

inline void out(std::nullptr_t, InitList<StringRef> err)
{
	outCommon(static_cast<const ModuleLoc *>(nullptr), err, false, false);
}

inline void outw(std::nullptr_t, InitList<StringRef> err)
{
	outCommon(static_cast<const ModuleLoc *>(nullptr), err, true, false);
}

// extras
void out(Stmt *stmt, InitList<StringRef> err);
void out(const lex::Lexeme &tok, InitList<StringRef> err);
void outw(Stmt *stmt, InitList<StringRef> err);
void outw(const lex::Lexeme &tok, InitList<StringRef> err);

} // namespace fer::err