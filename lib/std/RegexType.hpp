#pragma once

#include <regex>

#include "VM/Interpreter.hpp"

namespace fer
{

class VarRegex : public Var
{
	std::regex expr;

	using svmatch	 = std::match_results<StringRef::const_iterator>;
	using svsubmatch = std::sub_match<StringRef::const_iterator>;

public:
	VarRegex(const ModuleLoc *loc, StringRef exprStr,
		 std::regex::flag_type opts = std::regex_constants::ECMAScript);
	VarRegex(const ModuleLoc *loc, const std::regex &expr);
	~VarRegex();

	Var *copy(const ModuleLoc *loc);
	void set(Var *from);

	// loc can be nullptr if captures is not a VarVec (ie. no new strings will have to be
	// created)
	// If ignoreMatch is true, it will ignore the first match, ie. the regex equivalent in
	// string, and go directly for the capture groups if any.
	bool match(StringRef data, const ModuleLoc *loc = nullptr, Var *captures = nullptr,
		   bool ignoreMatch = false);
};

} // namespace fer