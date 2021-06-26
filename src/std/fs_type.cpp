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

#include "std/fs_type.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VAR_FILE /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

var_file_t::var_file_t(FILE *const file, const std::string &mode, const size_t &src_id,
		       const size_t &idx, const bool owner)
	: var_base_t(type_id<var_file_t>(), src_id, idx, false, false), m_file(file), m_mode(mode),
	  m_owner(owner)
{}
var_file_t::~var_file_t()
{
	if(m_owner) fclose(m_file);
}

var_base_t *var_file_t::copy(const size_t &src_id, const size_t &idx)
{
	return new var_file_t(m_file, m_mode, src_id, idx, false);
}

void var_file_t::set(var_base_t *from)
{
	if(m_owner) fclose(m_file);
	m_owner = false;
	m_file	= FILE(from)->get();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// VAR_FILE_ITERABLE ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

var_file_iterable_t::var_file_iterable_t(var_file_t *file, const size_t &src_id, const size_t &idx)
	: var_base_t(type_id<var_file_iterable_t>(), src_id, idx, false, false), m_file(file)
{
	var_iref(m_file);
}
var_file_iterable_t::~var_file_iterable_t()
{
	var_dref(m_file);
}

var_base_t *var_file_iterable_t::copy(const size_t &src_id, const size_t &idx)
{
	return new var_file_iterable_t(m_file, src_id, idx);
}
void var_file_iterable_t::set(var_base_t *from)
{
	var_dref(m_file);
	m_file = FILE_ITERABLE(from)->m_file;
	var_iref(m_file);
}

bool var_file_iterable_t::next(var_base_t *&val)
{
	char *line_ptr = NULL;
	size_t len     = 0;
	ssize_t read   = 0;

	if((read = getline(&line_ptr, &len, m_file->get())) != -1) {
		std::string line = line_ptr;
		free(line_ptr);
		while(line.back() == '\n') line.pop_back();
		while(line.back() == '\r') line.pop_back();
		val = make<var_str_t>(line);
		return true;
	}
	if(line_ptr) free(line_ptr);
	return false;
}