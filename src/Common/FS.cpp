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

#include "Common/FS.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <vector>

#include "Common/Env.hpp"

namespace fs
{
bool exists(const std::string &loc) { return access(loc.c_str(), F_OK) != -1; }

std::string abs_path(const std::string &loc, std::string *dir, const bool &dir_add_double_dot)
{
	static char abs[MAX_PATH_CHARS];
	static char abs_tmp[MAX_PATH_CHARS];
	realpath(loc.c_str(), abs);
	if(dir != nullptr) {
		std::string _abs = abs;
		*dir		 = _abs.substr(0, _abs.find_last_of('/'));
		if(dir_add_double_dot) {
			*dir += "/..";
			realpath(dir->c_str(), abs_tmp);
			*dir = abs_tmp;
		}
	}
	return abs;
}

std::string cwd()
{
	static char cwd[MAX_PATH_CHARS];
	if(getcwd(cwd, sizeof(cwd)) != NULL) {
		return cwd;
	}
	return "";
}

std::string home()
{
	static std::string _home = env::get("HOME");
	return _home;
}
} // namespace fs