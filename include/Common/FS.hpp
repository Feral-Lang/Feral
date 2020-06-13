/*
	MIT License

	Copyright (c) 2020 Feral Language repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#ifndef COMMON_FS_HPP
#define COMMON_FS_HPP

#include <vector>
#include <string>

#define MAX_PATH_CHARS 4096

namespace fs
{

bool exists( const std::string & loc );

std::string abs_path( const std::string & loc, std::string * dir = nullptr,
		      const bool & dir_add_double_dot = false );

std::string cwd();

std::string home();

}

#endif // COMMON_FS_HPP
