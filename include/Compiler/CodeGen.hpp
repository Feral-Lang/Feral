/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef COMPILER_CODE_GEN_HPP
#define COMPILER_CODE_GEN_HPP

#include "CodeGen/Internal.hpp"

namespace gen
{

bool generate( const ptree_t * ptree, bcode_t & bc );

}

#endif // COMPILER_CODE_GEN_HPP
