#include <charconv>

#include "VM/Interpreter.hpp"

using namespace fer;

Vector<Var *> _strSplit(Interpreter &vm, const ModuleLoc *loc, StringRef data, char delim);

static inline void trim(String &s);

size_t sizePow(size_t base, int exp);
size_t strToBin(StringRef str);

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *strSize(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const Map<String, AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, as<VarStr>(args[0])->get().size());
}

Var *strClear(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	as<VarStr>(args[0])->get().clear();
	return vm.getNil();
}

Var *strEmpty(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	return as<VarStr>(args[0])->get().size() == 0 ? vm.getTrue() : vm.getFalse();
}

Var *strFront(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	const String &str = as<VarStr>(args[0])->get();
	return str.size() == 0 ? (Var *)vm.getNil() : (Var *)vm.makeVar<VarStr>(loc, str.front());
}

Var *strBack(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const Map<String, AssnArgData> &assn_args)
{
	String &str = as<VarStr>(args[0])->get();
	return str.size() == 0 ? (Var *)vm.getNil() : (Var *)vm.makeVar<VarStr>(loc, str.back());
}

Var *strPush(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for string.push(), found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	String &src  = as<VarStr>(args[1])->get();
	String &dest = as<VarStr>(args[0])->get();
	if(src.size() > 0) dest += src;
	return args[0];
}

Var *strPop(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const Map<String, AssnArgData> &assn_args)
{
	String &str = as<VarStr>(args[0])->get();
	if(str.size() > 0) str.pop_back();
	return args[0];
}

Var *strIsChAt(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc, "expected first argument to be of type integer for index, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarStr>() && !args[2]->is<VarInt>()) {
		vm.fail(loc,
			"expected second argument to be of type "
			"string or integer for checking, found: ",
			vm.getTypeName(args[2]));
		return nullptr;
	}
	size_t pos   = mpz_get_ui(as<VarInt>(args[1])->get());
	String &dest = as<VarStr>(args[0])->get();
	if(pos >= dest.size()) {
		vm.fail(loc, "position ", pos, " is not within string of length: ", dest.size());
		return nullptr;
	}
	String chars;
	if(args[2]->is<VarInt>()) {
		chars = mpz_get_si(as<VarInt>(args[2])->get());
	} else if(args[2]->is<VarStr>()) {
		chars = as<VarStr>(args[2])->get();
	}
	return chars.find(dest[pos]) == String::npos ? vm.getFalse() : vm.getTrue();
}

Var *strSetAt(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc,
			"expected first argument to be of type integer for string.set(), found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarStr>()) {
		vm.fail(loc,
			"expected second argument to be of type string for string.set(), found: ",
			vm.getTypeName(args[2]));
		return nullptr;
	}
	size_t pos   = mpz_get_ui(as<VarInt>(args[1])->get());
	String &dest = as<VarStr>(args[0])->get();
	if(pos >= dest.size()) {
		vm.fail(loc, "position ", pos, " is not within string of length: ", dest.size());
		return nullptr;
	}
	String &src = as<VarStr>(args[2])->get();
	if(src.size() == 0) return args[0];
	dest[pos] = src[0];
	return args[0];
}

Var *strInsert(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc,
			"expected first argument to be of type "
			"integer for string.insert(), found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarStr>()) {
		vm.fail(loc,
			"expected second argument to be of type "
			"string for string.insert(), found: ",
			vm.getTypeName(args[2]));
		return nullptr;
	}
	size_t pos   = mpz_get_ui(as<VarInt>(args[1])->get());
	String &dest = as<VarStr>(args[0])->get();
	if(pos > dest.size()) {
		vm.fail(loc, "position ", pos, " is greater than string length: ", dest.size());
		return nullptr;
	}
	String &src = as<VarStr>(args[2])->get();
	dest.insert(dest.begin() + pos, src.begin(), src.end());
	return args[0];
}

Var *strErase(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc, "expected argument to be of type integer for string.erase(), found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	size_t pos  = mpz_get_ui(as<VarInt>(args[1])->get());
	String &str = as<VarStr>(args[0])->get();
	if(pos < str.size()) str.erase(str.begin() + pos);
	return args[0];
}

Var *strFind(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected argument to be of type str for string.find(), found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	String &str  = as<VarStr>(args[0])->get();
	String &what = as<VarStr>(args[1])->get();
	size_t pos   = str.find(what);
	if(pos == String::npos) {
		return vm.makeVar<VarInt>(loc, -1);
	}
	return vm.makeVar<VarInt>(loc, pos);
}

Var *strRFind(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected argument to be of type str for string.rfind(), found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	String &str  = as<VarStr>(args[0])->get();
	String &what = as<VarStr>(args[1])->get();
	size_t pos   = str.rfind(what);
	if(pos == String::npos) {
		return vm.makeVar<VarInt>(loc, -1);
	}
	return vm.makeVar<VarInt>(loc, pos);
}

Var *strSubstr(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(
		loc, "expected begin argument to be of type integer for string.substr(), found: ",
		vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarInt>()) {
		vm.fail(loc,
			"expected length argument to be of type "
			"integer for string.substr(), found: ",
			vm.getTypeName(args[2]));
		return nullptr;
	}
	size_t pos  = mpz_get_ui(as<VarInt>(args[1])->get());
	size_t len  = mpz_get_ui(as<VarInt>(args[2])->get());
	String &str = as<VarStr>(args[0])->get();
	return vm.makeVar<VarStr>(loc, str.substr(pos, len));
}

Var *strLast(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const Map<String, AssnArgData> &assn_args)
{
	return vm.makeVar<VarInt>(loc, as<VarStr>(args[0])->get().size() - 1);
}

Var *strTrim(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const Map<String, AssnArgData> &assn_args)
{
	String &str = as<VarStr>(args[0])->get();
	trim(str);
	return args[0];
}

Var *strUpper(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	String str = as<VarStr>(args[0])->get();
	size_t len = str.size();
	for(size_t i = 0; i < len; ++i) {
		str[i] = str[i] >= 'a' && str[i] <= 'z' ? str[i] ^ 0x20 : str[i];
	}
	return vm.makeVar<VarStr>(loc, str);
}

Var *strSplit(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	VarStr *str = as<VarStr>(args[0]);
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected string argument for delimiter, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	if(as<VarStr>(args[1])->get().size() == 0) {
		vm.fail(loc, "found empty delimiter for string split");
		return nullptr;
	}
	char delim  = as<VarStr>(args[1])->get()[0];
	VarVec *res = vm.makeVar<VarVec>(loc, 0, false);
	res->get()  = _strSplit(vm, loc, str->get(), delim);
	return res;
}

Var *strStartsWith(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		   const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected argument to be of type string for string.starts_with(), found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	const String &str = as<VarStr>(args[0])->get();
	String &with	  = as<VarStr>(args[1])->get();
	return vm.makeVar<VarBool>(loc, str.rfind(with, 0) == 0);
}

Var *strEndsWith(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		 const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc,
			"expected argument to be of type string for string.ends_with(), found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	const String &str = as<VarStr>(args[0])->get();
	String &with	  = as<VarStr>(args[1])->get();
	size_t pos	  = str.rfind(with);
	return vm.makeVar<VarBool>(loc, pos != String::npos && pos + with.size() == str.size());
}

Var *strFormat(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<String, AssnArgData> &assn_args)
{
	String str    = as<VarStr>(args[0])->get();
	size_t argctr = 1;
	for(size_t i = 0; i < str.size(); ++i) {
		if(str[i] != '{') continue;
		if(i > 0 && str[i - 1] == '\\') {
			str.erase(str.begin() + i - 1);
			--i;
			continue;
		}
		size_t start = i;
		++i;
		String expr;
		Var *base = nullptr;
		if(i < str.size() && str[i] == '}') {
			base = args[argctr++];
			incref(base);
		} else {
			while(i < str.size() && str[i] != '}') expr += str[i++];
			if(i == str.size()) {
				vm.fail(loc, "failed to find ending brace for eval expr: ", expr);
				return nullptr;
			}
			base = vm.eval(loc, expr);
			if(!base) {
				vm.fail(loc, "failed to evaluate expr: ", expr);
				return nullptr;
			}
		}
		Var *v = nullptr;
		Array<Var *, 1> tmp{base};
		if(!vm.callFn(loc, "str", v, tmp, {})) return nullptr;
		if(!v->is<VarStr>()) {
			vm.fail(loc,
				"'str' member call did not return a"
				" string, instead returned: ",
				vm.getTypeName(v));
			decref(v);
			decref(base);
			return nullptr;
		}
		String res = as<VarStr>(v)->get();
		decref(v);
		decref(base);
		str.erase(start, expr.size() + 2); // +2 for braces
		str.insert(str.begin() + start, res.begin(), res.end());
		i = start + res.size() - 1; // -1 for loop increment (++i)
	}
	return vm.makeVar<VarStr>(loc, std::move(str));
}

Var *hexStrToBinStr(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		    const Map<String, AssnArgData> &assn_args)
{
	static std::unordered_map<char, const char *> hextobin = {
	{'0', "0000"}, {'1', "0001"}, {'2', "0010"}, {'3', "0011"}, {'4', "0100"}, {'5', "0101"},
	{'6', "0110"}, {'7', "0111"}, {'8', "1000"}, {'9', "1001"}, {'a', "1010"}, {'b', "1011"},
	{'c', "1100"}, {'d', "1101"}, {'e', "1110"}, {'f', "1111"},
	};

	const String &str = as<VarStr>(args[0])->get();
	String bin;
	for(auto &ch : str) {
		char c = tolower(ch);
		if((c < '0' || c > '9') && (c < 'a' || c > 'f')) {
			vm.fail(loc, "expected hex string, found character: ", c);
			return nullptr;
		}
		bin += hextobin[c];
	}
	while(!bin.empty() && bin.front() == '0') bin.erase(bin.begin());
	return vm.makeVar<VarStr>(loc, bin);
}

Var *utf8CharFromBinStr(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
			const Map<String, AssnArgData> &assn_args)
{
	String str = as<VarStr>(args[0])->get();
	if(str.empty()) return vm.makeVar<VarStr>(loc, "");

	// reference: https://en.wikipedia.org/wiki/UTF-8#Encoding
	if(str.size() > 21) {
		vm.fail(loc, "UTF-8 cannot be more than 21 bytes, found bytes: ", str.size());
		return nullptr;
	}

	for(auto &c : str) {
		if(c == '0' || c == '1') continue;
		vm.fail(loc, "expected bin string, found character: ", c);
		return nullptr;
	}

	VarStr *res = vm.makeVar<VarStr>(loc, "");
	String &r   = as<VarStr>(res)->get();
	if(str.size() <= 7) {
		while(str.size() < 7) {
			str.insert(str.begin(), '0');
		}
		str.insert(str.begin(), '0');
		r = strToBin(str);
	} else if(str.size() <= 11) {
		while(str.size() < 11) {
			str.insert(str.begin(), '0');
		}
		r = strToBin("110" + str.substr(0, 5));
		r += strToBin("10" + str.substr(5));
	} else if(str.size() <= 16) {
		while(str.size() < 16) {
			str.insert(str.begin(), '0');
		}
		r = strToBin("1110" + str.substr(0, 4));
		r += strToBin("10" + str.substr(4, 6));
		r += strToBin("10" + str.substr(10));
	} else { // str.size() <= 21
		while(str.size() < 21) {
			str.insert(str.begin(), '0');
		}
		r = strToBin("11110" + str.substr(0, 3));
		r += strToBin("10" + str.substr(3, 6));
		r += strToBin("10" + str.substr(9, 6));
		r += strToBin("10" + str.substr(15));
	}

	return res;
}

// character (str[0]) to its ASCII (int)
Var *byt(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	 const Map<String, AssnArgData> &assn_args)
{
	const String &str = as<VarStr>(args[0])->get();
	if(str.empty()) return vm.makeVar<VarInt>(loc, 0);
	return vm.makeVar<VarInt>(loc, (unsigned char)str[0]);
}

// ASCII (int) to character (str)
Var *chr(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	 const Map<String, AssnArgData> &assn_args)
{
	u8 byt = (u8)mpz_get_si(as<VarInt>(args[0])->get());
	return vm.makeVar<VarStr>(loc, String(1, byt));
}

INIT_MODULE(Str)
{
	VarModule *mod = vm.getCurrModule();

	vm.addNativeTypeFn<VarStr>(loc, "len", strSize, 0);
	vm.addNativeTypeFn<VarStr>(loc, "clear", strClear, 0);
	vm.addNativeTypeFn<VarStr>(loc, "empty", strEmpty, 0);
	vm.addNativeTypeFn<VarStr>(loc, "front", strFront, 0);
	vm.addNativeTypeFn<VarStr>(loc, "back", strBack, 0);
	vm.addNativeTypeFn<VarStr>(loc, "push", strPush, 1);
	vm.addNativeTypeFn<VarStr>(loc, "pop", strPop, 0);
	vm.addNativeTypeFn<VarStr>(loc, "isChAt", strIsChAt, 2);
	vm.addNativeTypeFn<VarStr>(loc, "set", strSetAt, 2);
	vm.addNativeTypeFn<VarStr>(loc, "insert", strInsert, 2);
	vm.addNativeTypeFn<VarStr>(loc, "erase", strErase, 1);
	vm.addNativeTypeFn<VarStr>(loc, "find", strFind, 1);
	vm.addNativeTypeFn<VarStr>(loc, "rfind", strRFind, 1);
	vm.addNativeTypeFn<VarStr>(loc, "substrNative", strSubstr, 2);
	vm.addNativeTypeFn<VarStr>(loc, "lastIdx", strLast, 0);
	vm.addNativeTypeFn<VarStr>(loc, "trim", strTrim, 0);
	vm.addNativeTypeFn<VarStr>(loc, "upper", strUpper, 0);
	vm.addNativeTypeFn<VarStr>(loc, "splitNative", strSplit, 1);
	vm.addNativeTypeFn<VarStr>(loc, "startsWith", strStartsWith, 1);
	vm.addNativeTypeFn<VarStr>(loc, "endsWith", strEndsWith, 1);
	vm.addNativeTypeFn<VarStr>(loc, "fmt", strFormat, 0, true);
	vm.addNativeTypeFn<VarStr>(loc, "getBinStrFromHexStr", hexStrToBinStr, 0);
	vm.addNativeTypeFn<VarStr>(loc, "getUTF8CharFromBinStr", utf8CharFromBinStr, 0);

	vm.addNativeTypeFn<VarStr>(loc, "byt", byt, 0);
	vm.addNativeTypeFn<VarInt>(loc, "chr", chr, 0);

	return true;
}

Vector<Var *> _strSplit(Interpreter &vm, const ModuleLoc *loc, StringRef data, char delim)
{
	String temp;
	Vector<Var *> vec;

	for(auto c : data) {
		if(c == delim) {
			if(temp.empty()) continue;
			vec.push_back(vm.makeVarWithRef<VarStr>(loc, temp));
			temp.clear();
			continue;
		}

		temp += c;
	}

	if(!temp.empty()) vec.push_back(vm.makeVarWithRef<VarStr>(loc, temp));
	return vec;
}

// trim from start (in place)
static inline void ltrim(String &s)
{
	s.erase(s.begin(),
		std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
static inline void rtrim(String &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(),
		s.end());
}

// trim from both ends (in place)
static inline void trim(String &s)
{
	ltrim(s);
	rtrim(s);
}

size_t sizePow(size_t base, int exp)
{
	size_t result = 1;
	while(exp) {
		if(exp % 2) result *= base;
		exp /= 2;
		base *= base;
	}
	return result;
}

size_t strToBin(StringRef str)
{
	size_t exp = sizePow(2, str.size() - 1);
	size_t bin = 0;
	for(auto &c : str) {
		if(c == '1') {
			bin += exp;
		}
		exp /= 2;
	}
	return bin;
}