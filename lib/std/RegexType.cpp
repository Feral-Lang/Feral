#include "RegexType.hpp"

namespace fer
{

VarRegex::VarRegex(const ModuleLoc *loc, StringRef exprStr, std::regex::flag_type opts)
	: Var(loc, false, false), expr(exprStr.begin(), exprStr.end(), opts)
{}
VarRegex::VarRegex(const ModuleLoc *loc, const std::regex &expr)
	: Var(loc, false, false), expr(expr)
{}
VarRegex::~VarRegex() {}

Var *VarRegex::copy(const ModuleLoc *loc) { return new VarRegex(loc, expr); }
void VarRegex::set(Var *from) { expr = as<VarRegex>(from)->expr; }

bool VarRegex::match(StringRef data, const ModuleLoc *loc, Var *captures, bool ignoreMatch)
{
	svmatch results;
	bool found = std::regex_search(data.begin(), data.end(), results, expr);
	if(!captures) return found;
	size_t resCount = results.size();
	if(captures->is<VarVec>()) {
		VarVec *caps = as<VarVec>(captures);
		for(size_t i = ignoreMatch; i < resCount; ++i) {
			caps->push(new VarStr(loc, results[i].str()));
		}
	} else if(captures->is<VarStr>()) {
		VarStr *caps = as<VarStr>(captures);
		for(size_t i = ignoreMatch; i < resCount; ++i) {
			caps->get() += results[i].str();
		}
	}
	return found;
}

} // namespace fer