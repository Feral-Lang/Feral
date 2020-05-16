/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef COMMON_ENV_HPP
#define COMMON_ENV_HPP

#include <string>

namespace env
{

std::string get( const std::string & key );

std::string get_proc_path();

}

#endif // COMMON_ENV_HPP
