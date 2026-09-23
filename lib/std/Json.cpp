#include "Json.hpp"

#include <charconv>

#include "VM/VM.hpp"

namespace fer
{

const char *JsonTokenTypeStrs[] = {
    "Unknown", "Nil",   "Int",    "Float",  "String", "True",   "False",
    "Colon",   "Comma", "LBrack", "RBrack", "LBrace", "RBrace",
};

Tokenizer::Tokenizer(StringRef data)
    : data(data), pos(0), curr{JsonTokenType::UNKNOWN, "not started"}, done(false)
{}

void Tokenizer::setErr(String &&errStr)
{
    size_t start = std::min<size_t>(pos, 5);
    size_t end   = std::min<size_t>(data.size() - pos, 5);
    StringRef edata(data.begin() + pos - start, data.begin() + pos + end - 1);
    errorStr = std::move(errStr);
    errorStr += " at: ... ";
    errorStr += edata;
    errorStr += " ...";
}

bool Tokenizer::acceptAnyChar(StringRef val)
{
    for(size_t i = 0; i < val.size(); ++i) {
        if(acceptChar(val[i])) return true;
    }
    return false;
}

void Tokenizer::readConstStr()
{
    char quote        = data[pos - 1];
    size_t start      = pos;
    bool found        = false;
    int contBackslash = 0;
    while(pos < data.size()) {
        if(acceptAnyChar("\\")) {
            ++contBackslash;
            continue;
        }
        if(contBackslash == 0 && data[pos] == quote) {
            found = true;
            break;
        }
        ++pos;
        contBackslash = 0;
    }
    if(!found) {
        pos = start;
        return setErr("could not find ending quote for string");
    }
    curr = {JsonTokenType::STR, StringRef(data.begin() + start, data.begin() + pos)};
    ++pos;
}

void Tokenizer::readNum()
{
    size_t start = pos;
    if(data[pos] == '-') ++pos;
    bool flt = false;
    while(pos < data.size()) {
        char c = data[pos];
        if((c < '0' || c > '9') && c != '.') break;
        if(c == '.') {
            if(flt) {
                pos = start;
                return setErr("encountered multiple dots while reading number");
            }
            flt = true;
        }
        ++pos;
    }
    curr = {flt ? JsonTokenType::FLT : JsonTokenType::INT,
            StringRef(data.begin() + start, data.begin() + pos)};
}

void Tokenizer::readKeyword()
{
    size_t start = pos;
    while(pos < data.size()) {
        char c = data[pos];
        if((c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && (c < '0' || c > '9')) break;
        ++pos;
    }
    StringRef res(data.begin() + start, data.begin() + pos);
    if(res == "null") {
        curr = {JsonTokenType::NIL, ""};
    } else if(res == "true") {
        curr = {JsonTokenType::TTRUE, ""};
    } else if(res == "false") {
        curr = {JsonTokenType::TFALSE, ""};
    } else {
        pos = start;
        err("encountered invalid keyword: ", res);
    }
}

void Tokenizer::next()
{
    if(done) return;
    while(pos < data.size()) {
        if(acceptAnyChar("\n\r\t ")) continue;
        if(acceptAnyChar("\"")) readConstStr();
        else if(acceptAnyChar(":")) curr = {JsonTokenType::COLON, ""};
        else if(acceptAnyChar(",")) curr = {JsonTokenType::COMMA, ""};
        else if(acceptAnyChar("[")) curr = {JsonTokenType::LBRACK, ""};
        else if(acceptAnyChar("]")) curr = {JsonTokenType::RBRACK, ""};
        else if(acceptAnyChar("{")) curr = {JsonTokenType::LBRACE, ""};
        else if(acceptAnyChar("}")) curr = {JsonTokenType::RBRACE, ""};
        else if(data[pos] == '-' || (data[pos] >= '0' && data[pos] <= '9')) readNum();
        else readKeyword();
        return;
    }
    curr = {JsonTokenType::UNKNOWN, ""};
    done = true;
}

bool Tokenizer::expect(JsonTokenType::JsonTokType ty)
{
    if(!errorStr.empty()) return false;
    if(curr.type == ty) return true;
    err("expected json token: ", JsonTokenTypeStrs[ty],
        ", but found: ", JsonTokenTypeStrs[curr.type], "[",
        curr.val.empty() ? "<no data>" : curr.val, "]");
    return false;
}

bool Tokenizer::acceptn(JsonTokenType::JsonTokType ty)
{
    if(!errorStr.empty()) return false;
    if(curr.type == ty) {
        next();
        return true;
    }
    return false;
}

int64_t Tokenizer::asInt(int base)
{
    int64_t val;
    std::from_chars(curr.val.data(), curr.val.data() + curr.val.size(), val, base);
    return val;
}

double Tokenizer::asFlt()
{
    double val;
    std::from_chars(curr.val.data(), curr.val.data() + curr.val.size(), val);
    return val;
}

OStream &operator<<(OStream &os, const Tokenizer &t)
{
    os << JsonTokenTypeStrs[t.curr.type] << " [" << (t.curr.val.empty() ? "<no value>" : t.curr.val)
       << "]";
    return os;
}

Var *parseInternal(VirtualMachine &vm, ModuleLoc loc, Tokenizer &t)
{
    if(t.isErr()) {
        vm.fail(loc, "json parsing failed: ", t.getErr());
        return nullptr;
    }

    if(t.acceptn(JsonTokenType::NIL)) return vm.getNil();
    if(t.acceptn(JsonTokenType::TTRUE)) return vm.getTrue();
    if(t.acceptn(JsonTokenType::TFALSE)) return vm.getFalse();

    if(t.is(JsonTokenType::STR)) {
        VarStr *s = vm.makeVar<VarStr>(loc, t.asStr());
        t.next();
        return s;
    }
    if(t.is(JsonTokenType::INT)) {
        VarInt *i = vm.makeVar<VarInt>(loc, t.asInt());
        t.next();
        return i;
    }
    if(t.is(JsonTokenType::FLT)) {
        VarFlt *f = vm.makeVar<VarFlt>(loc, t.asFlt());
        t.next();
        return f;
    }

    if(t.acceptn(JsonTokenType::LBRACE)) {
        VarMap *m = vm.makeVar<VarMap>(loc, false, true);
        if(t.acceptn(JsonTokenType::RBRACE)) return m;
        do {
            if(t.isDone()) {
                vm.fail(loc, "json data ended before the end of map");
                vm.decVarRef(m);
                return nullptr;
            }
            if(!t.expect(JsonTokenType::STR)) goto errorMap;
            StringRef key = t.asStr();
            t.next();
            if(!t.acceptn(JsonTokenType::COLON)) goto errorMap;
            Var *v = parseInternal(vm, loc, t);
            if(!v) goto errorMap;
            m->setAttr(vm, key, v, true);
        } while(t.acceptn(JsonTokenType::COMMA));
        if(!t.acceptn(JsonTokenType::RBRACE)) goto errorMap;
        return m;
    errorMap:
        vm.fail(loc, "json map parsing failed: ", t.getErr());
        vm.decVarRef(m);
        return nullptr;
    }

    if(t.acceptn(JsonTokenType::LBRACK)) {
        VarVec *v = vm.makeVar<VarVec>(loc, 5, true);
        if(t.acceptn(JsonTokenType::RBRACK)) return v;
        do {
            Var *e = parseInternal(vm, loc, t);
            if(!e) goto errorVec;
            v->push(vm, e, true);
        } while(t.acceptn(JsonTokenType::COMMA));
        if(!t.acceptn(JsonTokenType::RBRACK)) goto errorVec;
        return v;
    errorVec:
        vm.fail(loc, "json vec parsing failed: ", t.getErr());
        vm.decVarRef(v);
        return nullptr;
    }

    auto &tok = t.get();
    vm.fail(loc, "unknown token type encountered: ", JsonTokenTypeStrs[tok.type], " [",
            (tok.val.empty() ? "<no value>" : tok.val), "]");
    return nullptr;
}

Var *parse(VirtualMachine &vm, ModuleLoc loc, Tokenizer &t)
{
    t.next();
    return parseInternal(vm, loc, t);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Functions /////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(feralJsonLoads, 1, false,
           "  fn(data) -> Var\n"
           "Parses the string `data` as JSON and creates its Feral representation.\n"
           "Returns the generated Feral object.")
{
    StringRef data = as<VarStr>(args[1])->getVal();
    Tokenizer t(data);
    return parse(vm, loc, t);
}

INIT_DLL(Json)
{
    vm.addLocal(loc, "loads", feralJsonLoads);
    return true;
}

} // namespace fer