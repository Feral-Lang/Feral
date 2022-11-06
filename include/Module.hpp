#pragma once

#include "Context.hpp"
#include "Core.hpp"

namespace fer
{
namespace lex
{
class Lexeme;
}
class ParserPassManager;

class Module
{
	Context &ctx;
	StringRef id;
	StringRef path;
	StringRef code;
	Vector<lex::Lexeme> tokens;
	Stmt *ptree;
	bool is_main_module;

public:
	Module(Context &ctx, StringRef id, StringRef path, StringRef code, bool is_main_module);
	~Module();

	bool tokenize();
	bool parseTokens();
	bool executeParseTreePasses(ParserPassManager &pm);

	inline StringRef getID() const { return id; }
	inline StringRef getPath() const { return path; }
	inline StringRef getCode() const { return code; }
	inline bool isMainModule() const { return is_main_module; }
	inline const Vector<lex::Lexeme> &getTokens() const { return tokens; }
	inline Stmt *&getParseTree() { return ptree; }

	void dumpTokens() const;
	void dumpParseTree() const;
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