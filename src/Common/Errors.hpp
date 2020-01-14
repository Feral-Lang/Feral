/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
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

	E_FAIL,
};

namespace err
{

size_t & code();
std::string & str();

void set( size_t err_code, const char * msg, ... );

}

#endif // COMMON_ERRORS_HPP
