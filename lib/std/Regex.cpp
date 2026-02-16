#include "Regex.hpp"

#include "VM/VM.hpp"

namespace fer
{

VarRegex::VarRegex(ModuleLoc loc, StringRef exprStr, std::regex::flag_type opts)
    : Var(loc, 0), expr(exprStr.begin(), exprStr.end(), opts)
{}
VarRegex::VarRegex(ModuleLoc loc, const std::regex &expr) : Var(loc, 0), expr(expr) {}
bool VarRegex::onSet(VirtualMachine &vm, Var *from)
{
    expr = as<VarRegex>(from)->expr;
    return true;
}
bool VarRegex::match(VirtualMachine &vm, StringRef data, ModuleLoc loc, Var *captures,
                     bool ignoreMatch)
{
    svmatch results;
    bool found = std::regex_search(data.begin(), data.end(), results, expr);
    if(!captures) return found;
    size_t resCount = results.size();
    if(captures->is<VarVec>()) {
        VarVec *caps = as<VarVec>(captures);
        for(size_t i = ignoreMatch; i < resCount; ++i) {
            caps->push(vm, vm.makeVar<VarStr>(loc, results[i].str()), true);
        }
    } else if(captures->is<VarStr>()) {
        VarStr *caps = as<VarStr>(captures);
        for(size_t i = ignoreMatch; i < resCount; ++i) { caps->getVal() += results[i].str(); }
    }
    return found;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(regexNew, 2, false,
           "  fn(regex, flags) -> Regex\n"
           "Creates and returns an instance of Regex using `regex` string and `flags`.")
{
    if(!args[1]->is<VarStr>()) {
        vm.fail(loc, "expected regex to be a string, found: ", vm.getTypeName(args[1]));
        return nullptr;
    }
    if(!args[2]->is<VarInt>()) {
        vm.fail(loc, "expected regex flags mask to be an int, found: ", vm.getTypeName(args[2]));
        return nullptr;
    }
    return vm.makeVar<VarRegex>(loc, as<VarStr>(args[1])->getVal(),
                                (std::regex::flag_type)as<VarInt>(args[2])->getVal());
}

FERAL_FUNC(regexMatch, 3, false,
           "  var.fn(data, dest, ignoreFirstMatch) -> Bool\n"
           "Matches the regex `var` with `data`. Stores any captured result in `dest`.\n"
           "Returns `true` if the match succeeds.\n"
           "`dest` can be either of string, vector, or nil, depending on if resulting match should "
           "be returned, and how.\n"
           "If `ignoreFirstMatch` is `true`, first match (usually full string `data`) is ignored "
           "from being put in `dest`.")
{
    if(!args[1]->is<VarStr>()) {
        vm.fail(loc, "expected regex target to be a string, found: ", vm.getTypeName(args[1]));
        return nullptr;
    }
    if(!args[2]->is<VarStr>() && !args[2]->is<VarVec>() && !args[2]->is<VarNil>()) {
        vm.fail(loc,
                "expected regex capture destination "
                "to be a string, vector, or nil, found: ",
                vm.getTypeName(args[1]));
        return nullptr;
    }
    if(!args[3]->is<VarBool>()) {
        vm.fail(loc, "expected ignore first match to be a bool, found: ", vm.getTypeName(args[1]));
        return nullptr;
    }
    StringRef target = as<VarStr>(args[1])->getVal();
    Var *matches     = nullptr;
    if(args[2]->is<VarVec>() || args[2]->is<VarStr>()) matches = args[2];
    bool ignoreMatch = as<VarBool>(args[3])->getVal();
    return as<VarRegex>(args[0])->match(vm, target, loc, matches, ignoreMatch) ? vm.getTrue()
                                                                               : vm.getFalse();
}

INIT_DLL(Regex)
{
    vm.addLocalType<VarRegex>(loc, "Regex", "The regex type for performing regex matching.");

    vm.addTypeFn<VarRegex>(loc, "matchNative", regexMatch);

    vm.addLocal(loc, "newNative", regexNew);

    vm.makeLocal<VarInt>(loc, "icase", "Ignore case.", std::regex_constants::icase);
    vm.makeLocal<VarInt>(loc, "nosubs",
                         "When a regular expression is matched against a character "
                         "container sequence, don't store sub-expression matches.",
                         std::regex_constants::nosubs);
    vm.makeLocal<VarInt>(loc, "optimize", "Optimize regex for matching speed.",
                         std::regex_constants::optimize);
    vm.makeLocal<VarInt>(loc, "collate", "Make the character ranges like [a-b] locale sensitive.",
                         std::regex_constants::collate);
    vm.makeLocal<VarInt>(loc, "ecmascript", "Use the ECMAScript regex format.",
                         std::regex_constants::ECMAScript);
    vm.makeLocal<VarInt>(loc, "basic", "Use the basic regex format.", std::regex_constants::basic);
    vm.makeLocal<VarInt>(loc, "extended", "Use the extended regex format.",
                         std::regex_constants::extended);
    vm.makeLocal<VarInt>(loc, "awk", "Use `awk` grammer.", std::regex_constants::awk);
    vm.makeLocal<VarInt>(loc, "grep", "Use `grep` grammer.", std::regex_constants::grep);
    vm.makeLocal<VarInt>(loc, "egrep", "Use `grep -E` grammer - consider newlines as whitespace.",
                         std::regex_constants::egrep);
    // Not sure why but multiline is undefined on Windows MSVC at the moment.
#if !defined(CORE_OS_WINDOWS) && (__cplusplus >= 201703L || !defined __STRICT_ANSI__)
    vm.makeLocal<VarInt>(loc, "multiline",
                         "Makes `^` and `$` work on each line and not the whole input.",
                         std::regex_constants::multiline);
#endif
    return true;
}

} // namespace fer