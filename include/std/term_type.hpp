/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
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