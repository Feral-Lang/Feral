#pragma once

#include "Args.hpp"
#include "Lexer.hpp"
#include "Parser/Passes/Base.hpp"

namespace fer
{

class RAIIParser
{
	ArgParser &args;
	Context ctx;

	ParserPassManager defaultparserpm;

	// as new sources are imported, they'll be pushed back
	Vector<StringRef> modulestack;

	// the iteration of this list will give the order of imports
	// this list does NOT contain the main module
	Vector<StringRef> moduleorder;

	Map<StringRef, Module *> modules;

public:
	RAIIParser(ArgParser &args);
	~RAIIParser();

	Module *createModule(const String &path, bool main_module);

	// generate the default set of passes to be run on the parse tree
	inline bool applyDefaultParserPasses(Module *mod)
	{
		return mod->executeParseTreePasses(defaultparserpm);
	}

	Module *getModule(StringRef path);

	// force ignores arg parser
	void dumpTokens(bool force);
	void dumpParseTree(bool force);

	inline bool hasModule(StringRef path) { return modules.find(path) != modules.end(); }
	inline const Vector<StringRef> &getModuleStack() { return modulestack; }
	inline ArgParser &getCommandArgs() { return args; }
	inline Context &getContext() { return ctx; }
};

} // namespace fer