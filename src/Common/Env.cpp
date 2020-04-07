/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <cstdlib>

#include "Env.hpp"

namespace env
{

std::string get( const std::string & key )
{
	const char * env = getenv( key.c_str() );
	return env == NULL ? "" : env;
}

}
