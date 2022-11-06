#include "RAIIParser.hpp"

#include <iostream>
#include <unistd.h>

#include "Error.hpp"
#include "FS.hpp"
#include "Parser/Passes/Base.hpp"
#include "Parser/Passes/Simplify.hpp"
#include "Utils.hpp"

namespace fer
{

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// RAIIParser /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

RAIIParser::RAIIParser(ArgParser &args) : args(args), ctx(), defaultparserpm(ctx)
{
	defaultparserpm.add<SimplifyParserPass>();
}
RAIIParser::~RAIIParser()
{
	for(auto &m : modules) delete m.second;
}

Module *RAIIParser::createModule(const String &_path, bool main_module)
{
	auto res = modules.find(_path);
	if(res != modules.end()) return res->second;
	String _code;
	if(!fs::read(_path, _code)) {
		err::out({"Failed to read source file: ", _path});
		return nullptr;
	}
	StringRef code = ctx.moveStr(std::move(_code));
	StringRef id   = ctx.strFrom(modulestack.size());
	StringRef path = ctx.strFrom(_path);
	Module *mod    = new Module(ctx, id, path, code, main_module);
	modulestack.push_back(path);
	modules[path] = mod;
	return mod;
}
Module *RAIIParser::getModule(StringRef path)
{
	auto res = modules.find(path);
	if(res != modules.end()) return res->second;
	return nullptr;
}

void RAIIParser::dumpTokens(bool force)
{
	if(!args.has("tokens") && !force) return;

	printf("-------------------------------------------------- Token(s) "
	       "--------------------------------------------------\n");
	for(auto file = modulestack.rbegin(); file != modulestack.rend(); ++file) {
		printf("\n\n");
		modules[*file]->dumpTokens();
	}
}
void RAIIParser::dumpParseTree(bool force)
{
	if(!args.has("parse") && !args.has("semantic") && !force) return;

	printf("-------------------------------------------------- Parse Tree(s) "
	       "--------------------------------------------------\n");
	for(auto file = modulestack.rbegin(); file != modulestack.rend(); ++file) {
		printf("\n\n");
		modules[*file]->dumpParseTree();
	}
}

} // namespace fer