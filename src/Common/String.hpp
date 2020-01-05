/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef COMMON_STRING_HPP
#define COMMON_STRING_HPP

#include <vector>
#include <string>

namespace str
{

std::vector< std::string > split( const std::string & data, const char delim, const bool keep_delim = false );

}

#endif // COMMON_STRING_HPP
