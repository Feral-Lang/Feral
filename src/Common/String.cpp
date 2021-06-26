/*
	MIT License

	Copyright (c) 2020 Feral Language repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "Common/String.hpp"

#include <string>
#include <vector>

namespace str
{
std::vector<std::string> split(const std::string &data, const char delim, const bool keep_delim)
{
	if(data.empty()) return {};
	std::string temp;
	std::vector<std::string> vec;

	for(auto c : data) {
		if(c == delim) {
			vec.push_back(std::string(1, c));
			if(temp.empty()) continue;
			vec.push_back(temp);
			temp.clear();
			continue;
		}

		temp += c;
	}

	if(!temp.empty()) vec.push_back(temp);
	return vec;
}

std::string stringify(const std::vector<std::string> &vec)
{
	std::string res = "[";
	for(auto &e : vec) {
		res += e + ", ";
	}
	if(vec.size() > 0) {
		res.pop_back();
		res.pop_back();
	}
	res += "]";
	return res;
}
} // namespace str
