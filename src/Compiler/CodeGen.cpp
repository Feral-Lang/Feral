/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "Compiler/CodeGen.hpp"

namespace gen
{

bool generate( const ptree_t * ptree, bcode_t & bc )
{
	// second arg = true to remove top level BLK_ADD & BLK_REM bytecode (made by BLOCK statement)
	return ptree->gen_code( bc, true );
}

}
