#include "Parser/Passes/Base.hpp"

namespace fer
{

ParserPass::ParserPass(const size_t &passid, Context &ctx) : passid(passid), ctx(ctx)
{
	ctx.addPass(passid, this);
}
ParserPass::~ParserPass() { ctx.remPass(passid); }

} // namespace fer