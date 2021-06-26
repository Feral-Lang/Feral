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

#ifndef COMMON_STRING_HPP
#define COMMON_STRING_HPP

#include <string>
#include <vector>

namespace str
{
std::vector<std::string> split(const std::string &data, const char delim,
			       const bool keep_delim = false);
std::string stringify(const std::vector<std::string> &vec);
} // namespace str

#endif // COMMON_STRING_HPP
