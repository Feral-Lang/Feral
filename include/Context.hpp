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
	List<ModuleLoc> modlocmem;
	List<Stmt *> stmtmem;
	Map<size_t, ParserPass *> passes;

public:
	Context();
	~Context();

	ModuleLoc *allocModuleLoc(Module *mod, size_t line, size_t col);

	template<typename T, typename... Args> T *allocStmt(Args &&...args)
	{
		T *res = new T(std::forward<Args>(args)...);
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