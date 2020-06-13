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

#ifndef FS_TYPE_HPP
#define FS_TYPE_HPP

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

#endif // FS_TYPE_HPP