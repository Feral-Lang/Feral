#include "Parser/ParseHelper.hpp"

namespace fer
{

ParseHelper::ParseHelper(Context &ctx, Module *mod, Vector<lex::Lexeme> &toks, size_t begin)
	: ctx(ctx), mod(mod), toks(toks), emptyloc(ctx.allocModuleLoc(mod, 0, 0)),
	  invalid(emptyloc, lex::INVALID), eof(emptyloc, lex::FEOF), idx(begin)
{}

lex::Lexeme &ParseHelper::peek(int offset)
{
	if(offset < 0 && idx < (-offset)) return eof;
	if(idx + offset >= toks.size()) return eof;
	return toks[idx + offset];
}

lex::TokType ParseHelper::peekt(int offset) const
{
	if(offset < 0 && idx < (-offset)) return eof.getTokVal();
	if(idx + offset >= toks.size()) return eof.getTokVal();
	return toks[idx + offset].getTokVal();
}

lex::Lexeme &ParseHelper::next()
{
	++idx;
	if(idx >= toks.size()) return eof;
	return toks[idx];
}

lex::TokType ParseHelper::nextt()
{
	++idx;
	if(idx >= toks.size()) return eof.getTokVal();
	return toks[idx].getTokVal();
}

lex::Lexeme &ParseHelper::prev()
{
	if(idx == 0) return invalid;
	--idx;
	return toks[idx];
}

lex::TokType ParseHelper::prevt()
{
	if(idx == 0) return invalid.getTokVal();
	--idx;
	return toks[idx].getTokVal();
}

const lex::Lexeme *ParseHelper::at(size_t idx) const
{
	if(idx >= toks.size()) return &invalid;
	return &toks[idx];
}

} // namespace fer