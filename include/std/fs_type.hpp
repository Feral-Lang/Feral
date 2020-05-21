/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef FS_HPP
#define FS_HPP

#include "../VM/VM.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// VAR_FILE /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class var_file_t : public var_base_t
{
	FILE * m_file;
	std::string m_mode;
	bool m_owner;
public:
	var_file_t( FILE * const file, const std::string & mode, const size_t & src_id,
		    const size_t & idx, const bool owner = true );
	~var_file_t();

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	inline FILE * const get() const { return m_file; }
	inline const std::string & mode() const { return m_mode; }
};
#define FILE( x ) static_cast< var_file_t * >( x )

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////// VAR_FILE_ITERABLE ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class var_file_iterable_t : public var_base_t
{
	var_file_t * m_file;
public:
	var_file_iterable_t( var_file_t * file, const size_t & src_id, const size_t & idx );
	~var_file_iterable_t();

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	bool next( var_base_t * & val );
};
#define FILE_ITERABLE( x ) static_cast< var_file_iterable_t * >( x )

#endif // FS_HPP