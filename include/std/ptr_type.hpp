/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef PTR_HPP
#define PTR_HPP

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

#endif // PTR_HPP