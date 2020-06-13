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

#ifndef PTR_TYPE_HPP
#define PTR_TYPE_HPP

#include "../VM/VM.hpp"

class var_ptr_t : public var_base_t
{
	var_base_t * m_val;
public:
	var_ptr_t( var_base_t * val, const size_t & src_id, const size_t & idx );
	~var_ptr_t();

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	void update( var_base_t * with );

	var_base_t * get();
};
#define PTR( x ) static_cast< var_ptr_t * >( x )

#endif // PTR_TYPE_HPP