#pragma once

#include "Core.hpp"

namespace fer
{

class Module;
class ModuleLoc;
class Stmt;
class ParserPass;

class Context
{
	List<String> stringmem;
	List<ModuleLoc> modlocmem;
	List<Stmt *> stmtmem;
	Map<size_t, ParserPass *> passes;

public:
	Context();
	~Context();

	StringRef strFrom(InitList<StringRef> strs);
	StringRef strFrom(const String &s);
	StringRef moveStr(String &&str);
	StringRef strFrom(int32_t i);
	StringRef strFrom(int64_t i);
	StringRef strFrom(uint32_t i);
	StringRef strFrom(size_t i);
#ifdef __APPLE__
	StringRef strFrom(uint64_t i);
#endif // __APPLE__

	ModuleLoc *allocModuleLoc(Module *mod, size_t line, size_t col);

	template<typename T, typename... Args> T *allocStmt(Args... args)
	{
		T *res = new T(args...);
		stmtmem.push_front(res);
		return res;
	}

	void addPass(const size_t &id, ParserPass *pass);
	void remPass(const size_t &id);
	ParserPass *getPass(const size_t &id);
	template<typename T>
	typename std::enable_if<std::is_base_of<ParserPass, T>::value, T *>::type getPass()
	{
		return static_cast<T *>(getPass(T::template genPassID<T>()));
	}
};

} // namespace fer