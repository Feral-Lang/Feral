/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
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
std::string stringify( const std::vector< std::string > & vec );

}

#endif // COMMON_STRING_HPP
