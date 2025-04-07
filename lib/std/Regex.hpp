#pragma once

#include "VM/VarTypes.hpp"

namespace fer
{

class VarRegex : public Var
{
	Regex expr;

	using svmatch	 = std::match_results<StringRef::const_iterator>;
	using svsubmatch = std::sub_match<StringRef::const_iterator>;

	Var *onCopy(MemoryManager &mem, ModuleLoc loc) override;
	void onSet(MemoryManager &mem, Var *from) override;

public:
	VarRegex(ModuleLoc loc, StringRef exprStr,
		 Regex::flag_type opts = std::regex_constants::ECMAScript);
	VarRegex(ModuleLoc loc, const Regex &expr);

	// loc can be nullptr if captures is not a VarVec (ie. no new strings will have to be
	// created)
	// If ignoreMatch is true, it will ignore the first match, ie. the regex equivalent in
	// string, and go directly for the capture groups if any.
	bool match(VirtualMachine &vm, StringRef data, ModuleLoc loc = {}, Var *captures = nullptr,
		   bool ignoreMatch = false);
};

} // namespace fer