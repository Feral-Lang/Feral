#include "RAIIParser.hpp"

#include <iostream>
#include <unistd.h>

#include "Error.hpp"
#include "FS.hpp"
#include "Parser/Passes/Base.hpp"
#include "Utils.hpp"

namespace fer
{

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// RAIIParser /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

RAIIParser::RAIIParser(ArgParser &args) : args(args), ctx() {}
RAIIParser::~RAIIParser()
{
	for(auto &m : modules) delete m.second;
}

Module *RAIIParser::createModule(const String &path, bool ismain)
{
	auto res = modules.find(path);
	if(res != modules.end()) return res->second;
	String code;
	if(!fs::read(path.c_str(), code)) {
		err::out(nullptr, "Failed to read source file: ", path);
		return nullptr;
	}
	Module *mod = new Module(ctx, modulestack.size(), path, std::move(code), ismain);
	modulestack.push_back(mod->getPath());
	modules[mod->getPath()] = mod;
	return mod;
}
Module *RAIIParser::getModule(StringRef path)
{
	auto res = modules.find(path);
	if(res != modules.end()) return res->second;
	return nullptr;
}

} // namespace fer