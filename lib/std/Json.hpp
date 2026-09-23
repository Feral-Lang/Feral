#pragma once

#include "VM/VarTypes.hpp"

namespace fer
{

namespace JsonTokenType
{
enum JsonTokType
{
    UNKNOWN,
    NIL,
    INT,
    FLT,
    STR,
    TTRUE,
    TFALSE,
    COLON,
    COMMA,
    LBRACK,
    RBRACK,
    LBRACE,
    RBRACE,
};
} // namespace JsonTokenType

struct JsonToken
{
    JsonTokenType::JsonTokType type;
    StringRef val;
};

class Tokenizer
{
    StringRef data;
    size_t pos;
    JsonToken curr;
    String errorStr;
    bool done;

    inline bool acceptChar(char val)
    {
        if(data[pos] == val) {
            ++pos;
            return true;
        }
        return false;
    }

    template<typename... Args> void err(Args &&...args)
    {
        String tmp;
        utils::appendToString(tmp, std::forward<Args>(args)...);
        return setErr(std::move(tmp));
    }

    void setErr(String &&errStr);

    bool acceptAnyChar(StringRef val);

    void readConstStr();
    void readNum();
    void readKeyword();

public:
    Tokenizer(StringRef data);

    inline JsonToken &get() { return curr; }
    inline bool is(JsonTokenType::JsonTokType ty) { return curr.type == ty; }
    inline bool isErr() { return !errorStr.empty(); }
    inline StringRef getErr() { return errorStr; }
    inline bool isDone() { return done; }
    inline StringRef asStr() { return curr.val; }

    void next();

    bool expect(JsonTokenType::JsonTokType ty);
    bool acceptn(JsonTokenType::JsonTokType ty);

    int64_t asInt(int base = 10);
    double asFlt();

    friend OStream &operator<<(OStream &os, const Tokenizer &t);
};

} // namespace fer