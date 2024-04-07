#include "Context.hpp"

#include "Module.hpp"
#include "Parser/Passes/Base.hpp"

// #define MEM_COUNT

namespace fer
{
Context::Context() {}
Context::~Context()
{
#if defined(MEM_COUNT)
	size_t loccount = 0, stmtcount = 0;
#endif

#if defined(MEM_COUNT)
	for(auto &loc : modlocmem) ++loccount;
#endif

	for(auto &s : stmtmem) {
#if defined(MEM_COUNT)
		++stmtcount;
#endif
		delete s;
	}

#if defined(MEM_COUNT)
	printf("Total deallocation:\nModLocs: %zu\nStmts: %zu\n", loccount, stmtcount);
#endif
}

ModuleLoc *Context::allocModuleLoc(Module *mod, size_t line, size_t col)
{
	modlocmem.emplace_front(mod, line, col);
	return &modlocmem.front();
}

void Context::addPass(const size_t &id, ParserPass *pass) { passes[id] = pass; }
void Context::remPass(const size_t &id)
{
	auto loc = passes.find(id);
	if(loc != passes.end()) passes.erase(loc);
}
ParserPass *Context::getPass(const size_t &id)
{
	auto loc = passes.find(id);
	if(loc != passes.end()) return loc->second;
	return nullptr;
}

} // namespace fer