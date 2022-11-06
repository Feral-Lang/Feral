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
	size_t s1 = 0, l1 = 0, s2 = 0, t1 = 0, v1 = 0;
	for(auto &s : stringmem) ++s1;
#endif
	for(auto &s : stmtmem) {
#ifdef MEM_COUNT
		++s2;
#endif
		delete s;
	}

#ifdef MEM_COUNT
	printf("Total deallocation:\nStrings: %zu\nModLocs:"
	       " %zu\nStmts: %zu\nTypes: %zu\nVals: %zu\n",
	       s1, l1, s2, t1, v1);
#endif
}

StringRef Context::strFrom(InitList<StringRef> strs)
{
	stringmem.push_front({});
	String &res = stringmem.front();
	for(auto &s : strs) {
		res += s;
	}
	return res;
}
StringRef Context::strFrom(const String &s)
{
	stringmem.push_front(s);
	return stringmem.front();
}
StringRef Context::moveStr(String &&str)
{
	stringmem.push_front(std::move(str));
	return stringmem.front();
}
StringRef Context::strFrom(int32_t i)
{
	stringmem.push_front(std::to_string(i));
	return stringmem.front();
}
StringRef Context::strFrom(int64_t i)
{
	stringmem.push_front(std::to_string(i));
	return stringmem.front();
}
StringRef Context::strFrom(uint32_t i)
{
	stringmem.push_front(std::to_string(i));
	return stringmem.front();
}
StringRef Context::strFrom(size_t i)
{
	stringmem.push_front(std::to_string(i));
	return stringmem.front();
}
#ifdef __APPLE__
StringRef Context::strFrom(uint64_t i)
{
	stringmem.push_front(std::to_string(i));
	return stringmem.front();
}
#endif // __APPLE__

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