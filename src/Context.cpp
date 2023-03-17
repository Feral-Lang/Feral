#include "Context.hpp"

#include "Module.hpp"
#include "Parser/Passes/Base.hpp"

// #define MEM_COUNT

namespace fer
{
Context::Context() {}
Context::~Context()
{
#ifdef MEM_COUNT
	size_t loccount = 0, stringcount = 0, stmtcount = 0;
#endif

#ifdef MEM_COUNT
	for(auto &str : strings) ++stringcount;
	for(auto &loc : modlocmem) ++loccount;
#endif

	for(auto &s : stmtmem) {
#ifdef MEM_COUNT
		++stmtcount;
#endif
		delete s;
	}

#ifdef MEM_COUNT
	printf("Total deallocation:\nModLocs: %zu\nStrings: %zu\nStmts: %zu\n", loccount,
	       stringcount, stmtcount);
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