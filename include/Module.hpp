#pragma once

#include "Context.hpp"
#include "Core.hpp"
#include "VM/Bytecode.hpp"

namespace fer
{
namespace lex
{
class Lexeme;
}
class Bytecode;

class Module
{
	Context &ctx;
	size_t id;
	StringRef dir;
	String path;
	String code;
	Vector<lex::Lexeme> tokens;
	Bytecode bc;
	Stmt *ptree;
	bool is_main_module;

public:
	Module(Context &ctx, size_t id, String &&path, String &&code, bool is_main_module);
	~Module();

	bool tokenize();
	bool parseTokens(bool expr_only = false);
	bool executeDefaultPasses();
	bool genCode(); // generate bytecode

	template<class T, typename... Args>
	typename std::enable_if<std::is_base_of<Pass, T>::value, bool>::type
	executePass(Args &&...args)
	{
		T pass(ctx, std::forward<Args>(args)...);
		return pass.visitTree(ptree);
	}

	inline size_t getID() const { return id; }
	inline StringRef getDir() const { return dir; }
	inline StringRef getPath() const { return path; }
	inline StringRef getCode() const { return code; }
	inline bool isMainModule() const { return is_main_module; }
	inline const Vector<lex::Lexeme> &getTokens() const { return tokens; }
	inline Bytecode &getBytecode() { return bc; }
	inline Stmt *&getParseTree() { return ptree; }

	void dumpTokens() const;
	void dumpParseTree() const;
	void dumpCode() const;
};

class ModuleLoc
{
	Module *mod;
	size_t line;
	size_t col;

public:
	ModuleLoc(Module *mod, size_t line, size_t col);

	String getLocStr() const;
	inline Module *getMod() const { return mod; }
	inline size_t getLine() const { return line; }
	inline size_t getCol() const { return col; }
};

} // namespace fer