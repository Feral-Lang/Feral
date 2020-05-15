/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef MAP_HPP
#define MAP_HPP

#include "../VM/VM.hpp"

class var_map_iterable_t : public var_base_t
{
	var_map_t * m_map;
	std::unordered_map< std::string, var_base_t * >::iterator m_curr;
public:
	var_map_iterable_t( var_map_t * map, const size_t & src_id, const size_t & idx );
	~var_map_iterable_t();

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	bool next( var_base_t * & val, const size_t & src_id, const size_t & idx );
};
#define MAP_ITERABLE( x ) static_cast< var_map_iterable_t * >( x )

#endif // MAP_HPP