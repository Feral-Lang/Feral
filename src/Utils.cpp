#include "Utils.hpp"

namespace fer
{

Vector<StringRef> stringDelim(StringRef str, StringRef delim)
{
	Vector<StringRef> res;

	size_t start = 0;
	size_t end   = str.find(delim);
	while(end != String::npos) {
		res.emplace_back(str.substr(start, end - start));
		start = end + delim.length();
		end   = str.find(delim, start);
	}
	res.emplace_back(str.substr(start, end));

	for(auto &s : res) {
		while(!s.empty() && s.front() == ' ') s = s.substr(0, 1);
		while(!s.empty() && s.back() == ' ') s = s.substr(s.size() - 1);
	}

	return res;
}

void appendRawString(String &res, StringRef from)
{
	res.reserve(res.size() + from.size());
	for(auto &e : from) {
		if(e == '\t') {
			res.push_back('\\');
			res.push_back('t');
		} else if(e == '\n') {
			res.push_back('\\');
			res.push_back('n');
		} else {
			res.push_back(e);
		}
	}
}

String vecToStr(Span<StringRef> items)
{
	String res = "[";
	for(auto i : items) {
		res += i;
		res += ", ";
	}
	if(!items.empty()) {
		res.pop_back();
		res.pop_back();
	}
	res += "]";
	return res;
}

String vecToStr(Span<String> items)
{
	String res = "[";
	for(auto i : items) {
		res += i;
		res += ", ";
	}
	if(!items.empty()) {
		res.pop_back();
		res.pop_back();
	}
	res += "]";
	return res;
}

} // namespace fer