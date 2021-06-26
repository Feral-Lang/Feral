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

#ifndef COMPILER_LOAD_FILE_HPP
#define COMPILER_LOAD_FILE_HPP

#include <unordered_map>

#include "../Common/Errors.hpp"
#include "../VM/SrcFile.hpp"

Errors fmod_read_code(const std::string &data, const std::string &src_dir,
		      const std::string &src_path, bcode_t &bc, const size_t &flags,
		      const bool is_main_src, const bool &expr_only, const size_t &begin_idx = 0,
		      const size_t &end_idx = -1);

srcfile_t *fmod_load(const std::string &src_file, const std::string &src_dir, const size_t &flags,
		     const bool is_main_src, Errors &err, const size_t &begin_idx = 0,
		     const size_t &end_idx = -1);

#endif // COMPILER_LOAD_FILE_HPP
