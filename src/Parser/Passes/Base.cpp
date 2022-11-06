#include "Parser/Passes/Base.hpp"

namespace fer
{

ParserPass::ParserPass(const size_t &passid, Context &ctx) : passid(passid), ctx(ctx)
{
	ctx.addPass(passid, this);
}
ParserPass::~ParserPass() { ctx.remPass(passid); }

ParserPassManager::ParserPassManager(Context &ctx) : ctx(ctx) {}
ParserPassManager::~ParserPassManager()
{
	for(auto &p : passes) delete p;
}
bool ParserPassManager::visit(Stmt *&ptree)
{
	for(auto &p : passes) {
		if(!p->visit(ptree, &ptree)) return false;
	}
	return true;
}

} // namespace fer