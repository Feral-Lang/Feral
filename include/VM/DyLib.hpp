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

#ifndef VM_DYN_LIB_HPP
#define VM_DYN_LIB_HPP

#include <string>
#include <unordered_map>

/* Wrapper class for dlfcn.h library */
class dyn_lib_t
{
	std::unordered_map<std::string, void *> m_handles;

public:
	dyn_lib_t();
	~dyn_lib_t();
	void *load(const std::string &file);
	void unload(const std::string &file);
	void *get(const std::string &file, const std::string &sym);
	inline bool fexists(const std::string &file)
	{
		return m_handles.find(file) != m_handles.end();
	}
};

#endif // VM_DYN_LIB_HPP
