/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef COMMON_IO_HPP
#define COMMON_IO_HPP

#include <cstdio>

namespace io
{
	void tadd( const bool show );
	void trem( const size_t num = 1 );
	void print( const bool has_next, const char * fmt, ... );
}

#endif // COMMON_IO_HPP
