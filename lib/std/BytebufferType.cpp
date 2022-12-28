#include "std/BytebufferType.hpp"

VarBytebuffer::VarBytebuffer(const ModuleLoc *loc, size_t bufsz)
	: Var(loc, typeID<VarBytebuffer>(), false, false), buffer(nullptr), bufsz(bufsz), buflen(0)
{
	if(bufsz > 0) buffer = (char *)malloc(bufsz);
}
VarBytebuffer::~VarBytebuffer()
{
	if(bufsz > 0) free(buffer);
}

Var *VarBytebuffer::copy(const ModuleLoc *loc)
{
	VarBytebuffer *newbuf = new VarBytebuffer(loc, bufsz);
	newbuf->set(this);
	return newbuf;
}

void VarBytebuffer::set(Var *from)
{
	VarBytebuffer *tmp = as<VarBytebuffer>(from);
	if(tmp->bufsz == 0) {
		if(bufsz > 0) free(buffer);
		bufsz = 0;
		return;
	}
	if(bufsz != tmp->bufsz) {
		if(bufsz == 0) buffer = (char *)malloc(tmp->bufsz);
		else buffer = (char *)realloc(buffer, tmp->bufsz);
	}
	memcpy(buffer, tmp->buffer, tmp->bufsz);
	bufsz  = tmp->bufsz;
	buflen = tmp->buflen;
}

void VarBytebuffer::resize(size_t newsz)
{
	if(newsz == 0) {
		if(bufsz > 0) free(buffer);
		bufsz = 0;
		return;
	}
	if(bufsz == 0) buffer = (char *)malloc(newsz);
	else buffer = (char *)realloc(buffer, newsz);
	bufsz = newsz;
}