#include "Module.hpp"

#include "FS.hpp"
#include "Lexer.hpp"
#include "Parser/Parser.hpp"
#include "Parser/Passes/Base.hpp"
#include "Parser/Passes/Codegen.hpp"
#include "Parser/Passes/Simplify.hpp"

namespace fer
{

Module::Module(Context &ctx, size_t id, String &&path, String &&code, bool is_main_module)
	: ctx(ctx), id(id), path(std::move(path)), code(std::move(code)), tokens(), ptree(nullptr),
	  is_main_module(is_main_module)
{
	dir = fs::parentDir(this->path);
}
Module::~Module() {}
bool Module::tokenize()
{
	lex::Tokenizer tokenizer(ctx, this);
	return tokenizer.tokenize(code, tokens);
}
bool Module::parseTokens(bool expr_only)
{
	ParseHelper p(ctx, this, tokens);
	Parser parser(ctx);
	return expr_only ? parser.parseExpr(p, ptree, false)
			 : parser.parseBlock(p, (StmtBlock *&)ptree, false);
}
bool Module::executeDefaultPasses()
{
	SimplifyPass p(ctx);
	if(!executePass<SimplifyPass>()) return false;
	return true;
}
bool Module::genCode() { return executePass<CodegenPass>(bc); }
void Module::dumpTokens() const
{
	std::cout << "Source: " << path << "\n";
	for(auto &t : tokens) {
		std::cout << t.str() << "\n";
	}
}
void Module::dumpParseTree() const
{
	std::cout << "Source: " << path << "\n";
	ptree->disp(false);
}
void Module::dumpCode() const
{
	std::cout << "Source: " << path << "\n";
	bc.dump(std::cout);
}

ModuleLoc::ModuleLoc(Module *mod, size_t line, size_t col) : mod(mod), line(line), col(col) {}

String ModuleLoc::getLocStr() const
{
	return std::to_string(line + 1) + ":" + std::to_string(col + 1);
}

} // namespace fer