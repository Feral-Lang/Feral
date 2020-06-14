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

#include "std/bytebuffer_type.hpp"

var_bytebuffer_t::var_bytebuffer_t( const size_t & buf_size, const size_t & src_id, const size_t & idx )
	: var_base_t( type_id< var_bytebuffer_t >(), src_id, idx, false, false ), m_size( buf_size ), m_buffer( nullptr )
{
	if( m_size > 0 ) m_buffer = ( char * )malloc( m_size );
}
var_bytebuffer_t::~var_bytebuffer_t()
{
	if( m_size > 0 ) free( m_buffer );
}

var_base_t * var_bytebuffer_t::copy( const size_t & src_id, const size_t & idx )
{
	var_bytebuffer_t * newbuf = new var_bytebuffer_t( m_size, src_id, idx );
	newbuf->set( this );
	return newbuf;
}

void var_bytebuffer_t::set( var_base_t * from )
{
	var_bytebuffer_t * tmp = BYTEBUFFER( from );
	if( tmp->m_size == 0 ) {
		if( m_size > 0 ) free( m_buffer );
		m_size = 0;
		return;
	}
	if( m_size == 0 ) m_buffer = ( char * )malloc( tmp->m_size );
	else m_buffer = ( char * )realloc( m_buffer, tmp->m_size );
	memcpy( m_buffer, tmp->m_buffer, tmp->m_size );
	m_size = tmp->m_size;
}

void var_bytebuffer_t::resize( const size_t & new_size )
{
	if( new_size == 0 ) {
		if( m_size > 0 ) free( m_buffer );
		m_size = 0;
		return;
	}
	if( m_size == 0 ) m_buffer = ( char * )malloc( new_size );
	else m_buffer = ( char * )realloc( m_buffer, new_size );
	m_size = new_size;
}

char *& var_bytebuffer_t::get_buf() { return m_buffer; }
const size_t & var_bytebuffer_t::get_size() { return m_size; }