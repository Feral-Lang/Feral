/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef COMMON_FS_HPP
#define COMMON_FS_HPP

#include <vector>
#include <string>

#define MAX_PATH_CHARS 4096

namespace fs
{

bool exists( const std::string & loc );

std::string abs_path( const std::string & loc, std::string * dir = nullptr );

std::string cwd();

std::string home();

}

#endif // COMMON_FS_HPP
