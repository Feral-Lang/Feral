/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "VM/Vars/Base.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// VAR_NIL //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_all_t::var_all_t( const size_t & src_id, const size_t & idx )
	: var_base_t( type_id< var_all_t >(), src_id, idx, false, false ) {}

var_base_t * var_all_t::copy( const size_t & src_id, const size_t & idx ) { return new var_all_t( src_id, idx ); }
void var_all_t::set( var_base_t * from ) {}
