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

#ifndef COMPILER_CODE_GEN_HPP
#define COMPILER_CODE_GEN_HPP

#include "CodeGen/Internal.hpp"

namespace gen
{

bool generate( const ptree_t * ptree, bcode_t & bc );

}

#endif // COMPILER_CODE_GEN_HPP
