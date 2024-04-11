#include "RAIIParser.hpp"

#include "Error.hpp"
#include "FS.hpp"

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

Module *RAIIParser::createModule(String &&path, bool ismain)
{
	auto res = modules.find(path);
	if(res != modules.end()) return res->second;
	String code;
	if(!fs::read(path.c_str(), code)) {
		err::out(nullptr, "Failed to read source file: ", path);
		return nullptr;
	}
	return createModule(std::move(path), std::move(code), ismain);
}

Module *RAIIParser::createModule(String &&path, String &&code, bool ismain)
{
	Module *mod = new Module(ctx, modulestack.size(), std::move(path), std::move(code), ismain);
	modules[mod->getPath()] = mod;
	return mod;
}

void RAIIParser::removeModule(StringRef path)
{
	// tiny optimization since most times removeModule should remove the most recent one
	// that is at least applicable for Interpreter::eval()
	if(!modulestack.empty() && modulestack.back() == path) {
		modulestack.pop_back();
	} else {
		auto loc = std::find(modulestack.begin(), modulestack.end(), path);
		if(loc != modulestack.end()) modulestack.erase(loc);
	}
	auto loc    = modules.find(path);
	Module *mod = loc->second;
	modules.erase(loc);
	delete mod;
}

Module *RAIIParser::getModule(StringRef path)
{
	auto res = modules.find(path);
	if(res != modules.end()) return res->second;
	return nullptr;
}

} // namespace fer