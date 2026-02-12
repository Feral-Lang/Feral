#include "Regex.hpp"

#include "VM/Interpreter.hpp"

namespace fer
{

VarRegex::VarRegex(ModuleLoc loc, StringRef exprStr, std::regex::flag_type opts)
    : Var(loc, 0), expr(exprStr.begin(), exprStr.end(), opts)
{}
VarRegex::VarRegex(ModuleLoc loc, const std::regex &expr) : Var(loc, 0), expr(expr) {}
Var *VarRegex::onCopy(MemoryManager &mem, ModuleLoc loc)
{
    return incVarRef(makeVar<VarRegex>(mem, loc, expr));
}
void VarRegex::onSet(MemoryManager &mem, Var *from) { expr = as<VarRegex>(from)->expr; }
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
            caps->push(vm.makeVar<VarStr>(loc, results[i].str()), true);
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

INIT_MODULE(Regex)
{
    VarModule *mod = vm.getCurrModule();

    vm.registerType<VarRegex>(loc, "Regex", "The regex type for performing regex matching.");

    vm.addNativeTypeFn<VarRegex>(loc, "matchNative", regexMatch);

    mod->addNativeFn(vm, "newNative", regexNew);

    mod->addNativeVar(vm, "icase", "Ignore case.",
                      vm.makeVar<VarInt>(loc, std::regex_constants::icase));
    mod->addNativeVar(vm, "nosubs",
                      "When a regular expression is matched against a character "
                      "container sequence, don't store sub-expression matches.",
                      vm.makeVar<VarInt>(loc, std::regex_constants::nosubs));
    mod->addNativeVar(vm, "optimize", "Optimize regex for matching speed.",
                      vm.makeVar<VarInt>(loc, std::regex_constants::optimize));
    mod->addNativeVar(vm, "collate", "Make the character ranges like [a-b] locale sensitive.",
                      vm.makeVar<VarInt>(loc, std::regex_constants::collate));
    mod->addNativeVar(vm, "ecmascript", "Use the ECMAScript regex format.",
                      vm.makeVar<VarInt>(loc, std::regex_constants::ECMAScript));
    mod->addNativeVar(vm, "basic", "Use the basic regex format.",
                      vm.makeVar<VarInt>(loc, std::regex_constants::basic));
    mod->addNativeVar(vm, "extended", "Use the extended regex format.",
                      vm.makeVar<VarInt>(loc, std::regex_constants::extended));
    mod->addNativeVar(vm, "awk", "Use `awk` grammer.",
                      vm.makeVar<VarInt>(loc, std::regex_constants::awk));
    mod->addNativeVar(vm, "grep", "Use `grep` grammer.",
                      vm.makeVar<VarInt>(loc, std::regex_constants::grep));
    mod->addNativeVar(vm, "egrep", "Use `grep -E` grammer - consider newlines as whitespace.",
                      vm.makeVar<VarInt>(loc, std::regex_constants::egrep));
    // Not sure why but multiline is undefined on Windows MSVC at the moment.
#if !defined(CORE_OS_WINDOWS) && (__cplusplus >= 201703L || !defined __STRICT_ANSI__)
    mod->addNativeVar(vm, "multiline",
                      "Makes `^` and `$` work on each line and not the whole input.",
                      vm.makeVar<VarInt>(loc, std::regex_constants::multiline));
#endif
    return true;
}

} // namespace fer