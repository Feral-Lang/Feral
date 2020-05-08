/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef COMPILER_LOAD_FILE_HPP
#define COMPILER_LOAD_FILE_HPP

#include <unordered_map>

#include "../Common/Errors.hpp"
#include "../VM/SrcFile.hpp"

Errors fmod_read_code( const std::string & data, const std::string & src_dir, const std::string & src_path,
		       bcode_t & bc, const size_t & flags, const bool is_main_src,
		       const bool & skip_expr_cols, const size_t & begin_idx = 0, const size_t & end_idx = -1 );

srcfile_t * fmod_load( const std::string & src_file, const size_t & flags, const bool is_main_src,
		       Errors & err, const bool & skip_expr_cols, const size_t & begin_idx = 0, const size_t & end_idx = -1 );

#endif // COMPILER_LOAD_FILE_HPP
