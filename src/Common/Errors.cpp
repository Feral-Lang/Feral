/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <cstring>
#include <cstdarg>

#include "Errors.hpp"

namespace err
{

size_t & code()
{
	static size_t ecode = E_OK;
	return ecode;
}

std::string & str()
{
	static std::string estr = "";
	return estr;
}

void set( size_t err_code, const char * msg, ... )
{
	static char err[ 2048 ];
	memset( err, 0, sizeof( err ) );

	va_list vargs;
	va_start( vargs, msg );
	vsprintf( err, msg, vargs );
	va_end( vargs );

	code() = err_code;
	str() = "error: " + std::string( err ) + "\n";
}

}
