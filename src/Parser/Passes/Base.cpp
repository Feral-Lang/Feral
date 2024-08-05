#include "Parser/Passes/Base.hpp"

namespace fer
{

Pass::Pass(const size_t &passid, Context &ctx) : passid(passid), ctx(ctx)
{
	ctx.addPass(passid, this);
}
Pass::~Pass() { ctx.remPass(passid); }

} // namespace fer