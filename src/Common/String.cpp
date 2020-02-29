/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <vector>
#include <string>

namespace str
{

std::vector< std::string > split( const std::string & data, const char delim, const bool keep_delim )
{
	std::string temp;
	std::vector< std::string > vec;

	for( auto c : data ) {
		if( c == delim ) {
			vec.push_back( std::string( 1, c ) );
			if( temp.empty() ) continue;
			vec.push_back( temp );
			temp.clear();
			continue;
		}

		temp += c;
	}

	if( !temp.empty() ) vec.push_back( temp );
	return vec;
}

std::string stringify( const std::vector< std::string > & vec )
{
	std::string res = "[";
	for( auto & e : vec ) {
		res += e + ", ";
	}
	if( vec.size() > 0 ) {
		res.pop_back();
		res.pop_back();
	}
	res += "]";
	return res;
}

}
