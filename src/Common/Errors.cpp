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

#include <cstring>
#include <cstdarg>

#include "Common/Errors.hpp"

namespace err
{

size_t & code()
{
	static size_t ecode = E_OK;
	return ecode;
}

size_t & val()
{
	static size_t _val = 0;
	return _val;
}

std::string & str()
{
	static std::string estr = "";
	return estr;
}

void set( const size_t & err_code, const size_t & err_val, const char * msg, ... )
{
	static char err[ 2048 ];
	memset( err, 0, sizeof( err ) );

	va_list vargs;
	va_start( vargs, msg );
	vsprintf( err, msg, vargs );
	va_end( vargs );

	code() = err_code;
	val() = err_val;
	str() = std::string( err );
}

}
