#pragma once

#include "VM/Interpreter.hpp"

namespace fer
{

class VarBytebuffer : public Var
{
	char *buffer;
	size_t bufsz;
	size_t buflen;

public:
	VarBytebuffer(const ModuleLoc *loc, size_t bufsz);
	~VarBytebuffer();

	Var *copy(const ModuleLoc *loc);
	void set(Var *from);

	void resize(size_t newsz);

	inline void setLen(size_t newlen) { buflen = newlen; }
	inline char *&getBuf() { return buffer; }
	inline size_t capacity() { return bufsz; }
	inline size_t len() { return buflen; }
};

} // namespace fer