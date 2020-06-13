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

#ifndef MAP_TYPE_HPP
#define MAP_TYPE_HPP

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

#endif // MAP_TYPE_HPP