#pragma once

#include "Core.hpp"

namespace fer
{
class Module;

class ModuleLoc
{
	Module *mod;
	size_t line;
	size_t col;

public:
	ModuleLoc(Module *mod, size_t line, size_t col);

	String getLocStr() const;
	inline Module *getMod() const { return mod; }
	inline size_t getLine() const { return line; }
	inline size_t getCol() const { return col; }
};

namespace err
{
void setMaxErrs(size_t max_err);

// loc can be nullptr in which case, error is shown directly (without location info)
void out(const ModuleLoc *loc, InitList<StringRef> err);
// equivalent to out(), but for warnings
void outw(const ModuleLoc *loc, InitList<StringRef> err);
} // namespace err
} // namespace fer