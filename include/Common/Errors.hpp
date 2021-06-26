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

#ifndef COMMON_ERRORS_HPP
#define COMMON_ERRORS_HPP

#include <string>

enum Errors
{
	E_OK,

	E_FILE_IO,
	E_FILE_EMPTY,

	E_LEX_FAIL,

	E_PARSE_FAIL,

	E_CODEGEN_FAIL,

	E_EXEC_FAIL,

	E_FAIL,
};

namespace err
{
size_t &code();
size_t &val();
std::string &str();

// err_val is for things like idx, etc.
void set(const size_t &err_code, const size_t &err_val, const char *msg, ...);
} // namespace err

#endif // COMMON_ERRORS_HPP
