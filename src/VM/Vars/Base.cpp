/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "../Memory.hpp"
#include "../VM.hpp"

#include "Base.hpp"

var_base_t::var_base_t( const size_t & type, const size_t & src_id, const size_t & idx )
	: m_type( type ), m_src_id( src_id ), m_idx( idx ), m_ref( 1 )
{}
var_base_t::~var_base_t()
{}

void * var_base_t::operator new( size_t sz )
{
	return mem::alloc( sz );
}
void var_base_t::operator delete( void * ptr, size_t sz )
{
	mem::free( ptr, sz );
}
