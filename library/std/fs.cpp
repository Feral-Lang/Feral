/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/


#include <unistd.h>

#include "../../src/VM/VM.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////// Enum values //////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum FileErrors
{
	FILE_OK,
	FILE_NOT_FOUND,
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// Classes //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// initialize this in the init_utils function
static int file_typeid;

class var_file_t : public var_base_t
{
	FILE * m_file;
	bool m_owner;
public:
	var_file_t( FILE * const file, const size_t & src_id, const size_t & idx, const bool owner = true );
	~var_file_t();

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	FILE * const get();
};
#define FILE( x ) static_cast< var_file_t * >( x )

var_file_t::var_file_t( FILE * const file, const size_t & src_id, const size_t & idx, const bool owner )
	: var_base_t( file_typeid, src_id, idx ), m_file( file ), m_owner( owner )
{}
var_file_t::~var_file_t()
{
	if( m_owner ) fclose( m_file );
}

var_base_t * var_file_t::copy( const size_t & src_id, const size_t & idx )
{
	m_owner = false;
	return new var_file_t( m_file, src_id, idx, true );
}

void var_file_t::set( var_base_t * from )
{
	if( m_owner ) fclose( m_file );
	m_owner = true;
	m_file = FILE( from )->get();
	FILE( from )->m_owner = false;
}

FILE * const var_file_t::get() { return m_file; }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////// Functions /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_base_t * fs_exists( vm_state_t & vm, const fn_data_t & fd )
{
	if( fd.args[ 1 ]->type() != VT_STR ) {
		vm.src_stack.back()->src()->fail( fd.idx, "expected string argument for path, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	return access( STR( fd.args[ 1 ] )->get().c_str(), F_OK ) != -1 ? vm.tru : vm.fals;
}

var_base_t * fs_open( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.src_stack.back()->src();
	if( fd.args[ 1 ]->type() != VT_STR ) {
		src->fail( fd.idx, "expected string argument for file name, found: %s",
			   vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	if( fd.args[ 2 ]->type() != VT_STR ) {
		src->fail( fd.idx, "expected string argument for file open mode, found: %s",
			   vm.type_name( fd.args[ 2 ]->type() ).c_str() );
		return nullptr;
	}
	const std::string & file_name = STR( fd.args[ 1 ] )->get();
	const std::string & mode = STR( fd.args[ 2 ] )->get();
	if( access( file_name.c_str(), F_OK ) == -1 ) {
		src->fail( fd.idx, "file '%s' is inaccessible", file_name.c_str() );
		return nullptr;
	}
	FILE * file = fopen( file_name.c_str(), mode.c_str() );
	if( !file ) {
		src->fail( fd.idx, "failed to open file '%s' in mode: %s",
			   file_name.c_str(), mode.c_str() );
		return nullptr;
	}
	return make< var_file_t >( file );
}

var_base_t * fs_file_readlines( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.src_stack.back()->src();
	FILE * const file = FILE( fd.args[ 0 ] )->get();
	char * line_ptr = NULL;
	size_t len = 0;
	ssize_t read = 0;

	std::vector< var_base_t * > lines;
	while( ( read = getline( & line_ptr, & len, file ) ) != -1 ) {
		std::string line = line_ptr;
		while( line.back() == '\n' ) line.pop_back();
		while( line.back() == '\r' ) line.pop_back();
		lines.push_back( new var_str_t( line, fd.src_id, fd.idx ) );
	}
	if( line_ptr ) free( line_ptr );
	fseek( file, 0, SEEK_SET );

	return make< var_vec_t >( lines );
}

REGISTER_MODULE( fs )
{
	var_src_t * src = vm.src_stack.back();
	const std::string & src_name = src->src()->path();

	// get the type id for file type (register_type)
	file_typeid = vm.register_new_type();

	src->add_nativefn( "exists", fs_exists, { "" } );

	src->add_nativefn( "open_native", fs_open, { "", "" }, {} );

	vm.add_typefn( file_typeid, "readlines", new var_fn_t( src_name, {}, {}, { .native = fs_file_readlines }, 0, 0 ), false );

	return true;
}
