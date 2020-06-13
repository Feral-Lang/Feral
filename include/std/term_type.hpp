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

#ifndef TERM_TYPE_HPP
#define TERM_TYPE_HPP

#include <termios.h>

#include "../VM/VM.hpp"

class var_term_t : public var_base_t
{
	struct termios m_term;
public:
	var_term_t( const struct termios & term, const size_t & src_id, const size_t & idx );

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	inline struct termios & get() { return m_term; }
};
#define TERM( x ) static_cast< var_term_t * >( x )

#endif // TERM_TYPE_HPP