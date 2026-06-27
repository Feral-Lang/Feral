#pragma once

#include "Status.hpp"

namespace fer::args
{

class FER_API ArgInfo
{
    // Name for the argument
    String name;
    // Help string for the argument.
    String help;
    // Options (short/long).
    // If none are provided, this is argument is considered a positional argument.
    // The position of which is decided by the order if addition to the ArgParser.
    // Positional args are always required.
    Vector<String> opts;
    // Parsed value for the argument (stored after parsing).
    StringRef val;
    // Is this argument required.
    bool reqd;
    // Is a value for this argument required?
    bool valReqd;
    // Was this arg found during parsing?
    bool found;

public:
    ArgInfo();

    inline ArgInfo &setName(StringRef _name)
    {
        name = _name;
        return *this;
    }
    inline ArgInfo &setHelp(StringRef val)
    {
        help = val;
        return *this;
    }
    inline ArgInfo &setVal(StringRef _val)
    {
        val = _val;
        return *this;
    }
    template<typename... Args> ArgInfo &addOpts(Args... addOpts)
    {
        int tmp[] = {(opts.emplace_back(addOpts), 0)...};
        return *this;
    }
    inline ArgInfo &setReqd(bool req)
    {
        reqd = req;
        return *this;
    }
    inline ArgInfo &setValReqd(bool req)
    {
        valReqd = req;
        return *this;
    }
    inline ArgInfo &setFound(bool val)
    {
        found = val;
        return *this;
    }

    inline StringRef getName() { return name; }
    inline StringRef getHelp() { return help; }
    inline Span<String> getOpts() { return opts; }
    inline StringRef getOpt(size_t idx) { return opts[idx]; }
    inline StringRef getVal() { return val; }
    inline bool isRequired() { return reqd; }
    inline bool isPositional() { return opts.empty(); }
    inline bool requiresValue() { return valReqd || isPositional(); }
    inline bool isFound() { return found; }

    inline bool hasOpt(StringRef opt)
    {
        return std::find(opts.begin(), opts.end(), opt) != opts.end();
    }
};

// Note that `--` can be used to
class FER_API ArgParser
{
    Vector<StringRef> argv;
    // What/How to parse.
    Vector<ArgInfo> argDefs;
    // Last arg to be parsed. Only positional args can be a lastParsedArg.
    // Any arg after will count as if `--` has been applied after this arg.
    String lastParsedArg;
    // Index of arg right after `--` or lastParsedArg.
    // All parameters after that will not be parsed.
    size_t passThroughFrom;

    // Common between constructors.
    void init();

public:
    ArgParser(Span<StringRef> args);
    ArgParser(int argc, const char **argv);

    ArgInfo &addArg(StringRef argname);
    ArgInfo *getArg(StringRef argname);

    Status<bool> parse();
    void printHelp(OStream &os);

    // retrieve info
    bool has(StringRef argname);
    StringRef getValue(StringRef argname);
    Span<StringRef> getPassthrough();

    inline void setLastArg(StringRef argname) { lastParsedArg = argname; }
    inline Span<ArgInfo> getArgDefs() { return argDefs; }
};

} // namespace fer::args