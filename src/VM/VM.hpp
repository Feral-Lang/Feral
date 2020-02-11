/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef VM_VM_HPP
#define VM_VM_HPP

#include <string>
#include <cstdlib>

#include "SrcFile.hpp"
#include "OpCodes.hpp"

namespace vm
{

inline srcfile_t src_new( const std::string & dir, const std::string & path,
			  const bool is_main_src = false )
{
	static size_t id = 0;
	auto src = srcfile_t( id++, dir, path, is_main_src );
	return src;
}

}

#endif // VM_VM_HPP
