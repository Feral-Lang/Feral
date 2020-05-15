/*
	Copyright (c) 2019, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <unistd.h>

#include "Common/Env.hpp"
#include "Common/FS.hpp"

namespace fs
{

bool exists( const std::string & loc )
{
	return access( loc.c_str(), F_OK ) != -1;
}

std::string abs_path( const std::string & loc, std::string * dir )
{
	static char abs[ MAX_PATH_CHARS ];
	realpath( loc.c_str(), abs );
	if( dir != nullptr ) {
		std::string _abs = abs;
		* dir = _abs.substr( 0, _abs.find_last_of( '/' ) );
	}
	return abs;
}

std::string cwd()
{
	static char cwd[ MAX_PATH_CHARS ];
	if( getcwd( cwd, sizeof( cwd ) ) != NULL ) {
		return cwd;
	}
	return "";
}

std::string home()
{
	static std::string _home = env::get( "HOME" );
	return _home;
}

}