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

	// as new sources are imported, they'll be pushed back
	Vector<StringRef> modulestack;

	Map<StringRef, Module *> modules;

public:
	RAIIParser(ArgParser &args);
	~RAIIParser();

	Module *createModule(String &&path, bool ismain);

	Module *getModule(StringRef path);

	inline bool hasModule(StringRef path) { return modules.find(path) != modules.end(); }
	inline const Vector<StringRef> &getModuleStack() { return modulestack; }
	inline ArgParser &getCommandArgs() { return args; }
	inline Context &getContext() { return ctx; }
};

} // namespace fer