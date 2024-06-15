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

bool VarRegex::match(StringRef data, const ModuleLoc *loc, VarVec *captures)
{
	svmatch results;
	bool found = std::regex_search(data.begin(), data.end(), results, expr);
	if(!captures) return found;
	for(const svsubmatch &res : results) {
		captures->push(new VarStr(loc, res.str()));
	}
	return found;
}

} // namespace fer