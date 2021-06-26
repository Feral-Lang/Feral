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

#include "Common/IO.hpp"

#include <cstdarg>
#include <vector>

static std::vector<bool> &_tab()
{
	static std::vector<bool> tabs;
	return tabs;
}

static void _tab_apply(const bool has_next)
{
	for(size_t i = 0; i < _tab().size(); ++i) {
		if(i == _tab().size() - 1) {
			if(has_next) fprintf(stdout, "  ├─");
			else
				fprintf(stdout, "  └─");
		} else {
			if(_tab()[i]) fprintf(stdout, "  │");
			else
				fprintf(stdout, "   ");
		}
	}
}

void io::tadd(const bool show)
{
	_tab().push_back(show);
}

void io::trem(const size_t num)
{
	if(num > _tab().size()) return;
	for(size_t i = 0; i < num; ++i) _tab().pop_back();
}

void io::print(const bool has_next, const char *fmt, ...)
{
	_tab_apply(has_next);
	va_list args;
	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
}
