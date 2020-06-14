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

#ifndef BYTEBUFFER_TYPE_HPP
#define BYTEBUFFER_TYPE_HPP

#include "../VM/VM.hpp"

class var_bytebuffer_t : public var_base_t
{
	char * m_buffer;
	size_t m_size;
public:
	var_bytebuffer_t( const size_t & buf_size, const size_t & src_id, const size_t & idx );
	~var_bytebuffer_t();

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	void resize( const size_t & new_size );

	char *& get_buf();
	const size_t & get_size();
};
#define BYTEBUFFER( x ) static_cast< var_bytebuffer_t * >( x )

#endif // BYTEBUFFER_TYPE_HPP