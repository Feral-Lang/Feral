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

bool VarRegex::match(StringRef data, const ModuleLoc *loc, Var *captures)
{
	svmatch results;
	bool found = std::regex_search(data.begin(), data.end(), results, expr);
	if(!captures) return found;
	if(captures->is<VarVec>()) {
		VarVec *caps = as<VarVec>(captures);
		for(const svsubmatch &res : results) {
			caps->push(new VarStr(loc, res.str()));
		}
	} else if(captures->is<VarStr>()) {
		VarStr *caps = as<VarStr>(captures);
		for(const svsubmatch &res : results) {
			caps->get() += res.str();
		}
	}
	return found;
}

} // namespace fer