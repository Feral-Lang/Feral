/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef VEC_HPP
#define VEC_HPP

#include "../VM/VM.hpp"

class var_vec_iterable_t : public var_base_t
{
	var_vec_t * m_vec;
	size_t m_curr;
public:
	var_vec_iterable_t( var_vec_t * vec, const size_t & src_id, const size_t & idx );
	~var_vec_iterable_t();

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	bool next( var_base_t * & val );
};
#define VEC_ITERABLE( x ) static_cast< var_vec_iterable_t * >( x )

#endif // VEC_HPP