#include "Args.hpp"

namespace fer::args
{

ArgInfo::ArgInfo() : reqd(false), valReqd(false), found(false) {}

ArgParser::ArgParser(Span<StringRef> args) : passThroughFrom(-1)
{
    init();
    for(auto &a : args) { argv.emplace_back(a); }
}
ArgParser::ArgParser(int argc, const char **argv)
{
    init();
    for(int i = 0; i < argc; ++i) this->argv.emplace_back(argv[i]);
}

void ArgParser::init()
{
    passThroughFrom = -1;
    addArg("help").addOpts("--help", "-h").setHelp("prints help information for program");
}

ArgInfo &ArgParser::addArg(StringRef argname)
{
    argDefs.emplace_back();
    ArgInfo &inf = argDefs.back();
    inf.setName(argname);
    return inf;
}

ArgInfo *ArgParser::getArg(StringRef argname)
{
    for(size_t i = 0; i < argDefs.size(); ++i) {
        if(argDefs[i].getName() == argname) return &argDefs[i];
    }
    return nullptr;
}

Status<bool> ArgParser::parse()
{
    String expectKey;
    size_t expectingValIdx = -1;
    bool stopParsing       = false;
    for(size_t i = 1; i < argv.size(); ++i) {
        StringRef arg = argv[i];
        bool isOpt    = !arg.empty() && arg[0] == '-';
        if(expectingValIdx != -1) {
            if(isOpt) {
                return Status(false, "expected value for arg: ", argDefs[expectingValIdx].getName(),
                              " but found option: ", arg);
            }
            argDefs[expectingValIdx].setVal(arg);
            argDefs[expectingValIdx].setFound(true);
            expectingValIdx = -1;
            if(stopParsing) {
                passThroughFrom = i + 1;
                break;
            }
            continue;
        }
        if(arg == "--") {
            passThroughFrom = i + 1;
            break;
        }
        size_t matched = -1;
        for(size_t j = 0; j < argDefs.size(); ++j) {
            auto &a = argDefs[j];
            if(isOpt && a.isPositional()) continue;
            if(isOpt) {
                if(!a.hasOpt(arg)) continue;
                if(a.isFound()) { return Status(false, "found a repeated option: ", arg); }
                matched = j;
                if(a.requiresValue()) expectingValIdx = j;
                // set found when value is received if required
                a.setFound(!a.requiresValue());
                break;
            }
            if(!a.isPositional() || a.isFound()) continue;
            // positional arg
            if(!a.isFound()) {
                a.setVal(arg);
                a.setFound(true);
                matched = j;
                break;
            }
            return Status(false, "found a repeated argument: ", arg);
        }
        if(matched == -1) { return Status(false, "invalid argument: ", arg, ", use --help"); }
        stopParsing = argDefs[matched].getName() == lastParsedArg;
        if(stopParsing && expectingValIdx == -1) {
            passThroughFrom = i + 1;
            break;
        }
    }
    if(expectingValIdx != -1) {
        // error: expected value for arg: argDefs[expectingValIdx].name,
        // but no more args to parse
        return Status(false, "expected value for argument: ", argDefs[expectingValIdx].getName(),
                      ", but there are no more args to parse");
    }
    for(auto &a : argDefs) {
        if(!a.isFound() && a.isRequired()) {
            // error: Required argument: a, was not found.
            return Status(false, "required argument: ", a.getName(), " was not found");
        }
    }
    return Status(true);
}

void ArgParser::printHelp(OStream &os)
{
    os << "Usage: " << argv[0];
    for(auto &a : argDefs) {
        if(!a.isPositional()) continue;
        os << " " << a.getName();
    }
    os << " <option args>\n\n";
    for(auto &a : argDefs) {
        auto opts = a.getOpts();
        for(size_t i = 0; i < opts.size(); ++i) {
            os << opts[i];
            if(i != opts.size() - 1) os << "/";
        }
        if(opts.empty()) os << a.getName() << " (positional)";
        if(a.isRequired()) os << " (required)";
        else os << " (optional)";
        if(a.requiresValue()) os << " (requires value)";
        os << "\t\t" << a.getHelp() << "\n";
    }
}

bool ArgParser::has(StringRef argname)
{
    for(auto &a : argDefs) {
        if(a.getName() != argname) continue;
        return a.isFound();
    }
    return false;
}
StringRef ArgParser::getValue(StringRef argname)
{
    for(auto &a : argDefs) {
        if(a.getName() != argname) continue;
        return a.getVal();
    }
    return "";
}
Span<StringRef> ArgParser::getPassthrough()
{
    if(passThroughFrom == -1 || passThroughFrom >= argv.size()) return {};
    return Span<StringRef>(argv.begin() + passThroughFrom, argv.end());
}

} // namespace fer::args