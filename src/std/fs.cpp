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

#include <regex>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>

#include "VM/VM.hpp"

#include "std/bytebuffer_type.hpp"
#include "std/fs_type.hpp"

enum WalkEntry {
	FILES = 1 << 0,
	DIRS = 1 << 1,
	RECURSE = 1 << 2,
};

void get_entries_internal( const std::string & dir_str, std::vector< var_base_t * > & v,
			   const size_t & flags, const int src_id, const int idx,
			   const std::regex & regex );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////// Functions /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_base_t * fs_exists( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected string argument for path, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	return access( STR( fd.args[ 1 ] )->get().c_str(), F_OK ) != -1 ? vm.tru : vm.fals;
}

var_base_t * fs_open( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected string argument for file name, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( !fd.args[ 2 ]->istype< var_str_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected string argument for file open mode, found: %s",
			 vm.type_name( fd.args[ 2 ] ).c_str() );
		return nullptr;
	}
	const std::string & file_name = STR( fd.args[ 1 ] )->get();
	const std::string & mode = STR( fd.args[ 2 ] )->get();
	FILE * file = fopen( file_name.c_str(), mode.c_str() );
	if( !file ) {
		vm.fail( fd.src_id, fd.idx, "failed to open file '%s' in mode: %s",
			 file_name.c_str(), mode.c_str() );
		return nullptr;
	}
	return make< var_file_t >( file, mode );
}

var_base_t * fs_file_lines( vm_state_t & vm, const fn_data_t & fd )
{
	FILE * const file = FILE( fd.args[ 0 ] )->get();
	char * line_ptr = NULL;
	size_t len = 0;
	ssize_t read = 0;

	std::vector< var_base_t * > lines;
	while( ( read = getline( & line_ptr, & len, file ) ) != -1 ) {
		std::string line = line_ptr;
		while( line.back() == '\n' || line.back() == '\r' ) line.pop_back();
		lines.push_back( new var_str_t( line, fd.src_id, fd.idx ) );
	}
	if( line_ptr ) free( line_ptr );
	fseek( file, 0, SEEK_SET );

	return make< var_vec_t >( lines, false );
}

var_base_t * fs_file_seek( vm_state_t & vm, const fn_data_t & fd )
{
	FILE * const file = FILE( fd.args[ 0 ] )->get();
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected int argument for file seek position, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( !fd.args[ 2 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected int argument for file seek origin, found: %s",
			 vm.type_name( fd.args[ 2 ] ).c_str() );
		return nullptr;
	}
	long pos = mpz_get_si( INT( fd.args[ 1 ] )->get() );
	int origin = mpz_get_si( INT( fd.args[ 2 ] )->get() );
	return make< var_int_t >( fseek( file, pos, origin ) );
}

var_base_t * fs_file_each_line( vm_state_t & vm, const fn_data_t & fd )
{
	return make< var_file_iterable_t >( FILE( fd.args[ 0 ] ) );
}

var_base_t * fs_file_read_blocks( vm_state_t & vm, const fn_data_t & fd )
{
	FILE * const file = FILE( fd.args[ 0 ] )->get();

	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected string argument for block begin location, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( !fd.args[ 2 ]->istype< var_str_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected string argument for block end location, found: %s",
			 vm.type_name( fd.args[ 2 ] ).c_str() );
		return nullptr;
	}

	const std::string & begin = STR( fd.args[ 1 ] )->get();
	const std::string & end = STR( fd.args[ 2 ] )->get();
	bool inside_block = false;

	char * line_ptr = NULL;
	size_t len = 0;
	ssize_t read = 0;

	std::string line;
	std::string block_content;
	std::vector< var_base_t * > blocks;
	while( ( read = getline( & line_ptr, & len, file ) ) != -1 ) {
		line = line_ptr;
	begin_fetch:
		if( line.empty() ) continue;
		if( !inside_block ) {
			size_t pos = line.find( begin );
			if( pos == std::string::npos ) continue;
			inside_block = true;
			if( pos + begin.size() > line.size() ) continue;
			else line = line.substr( pos + begin.size() );
			goto begin_fetch;
		}
		size_t pos = line.find( end );
		if( pos == std::string::npos ) { block_content += line; continue; }
		block_content += line.substr( 0, pos );
		if( pos + end.size() <= line.size() ) line = line.substr( pos + end.size() );
		else line.clear();
		inside_block = false;
		blocks.push_back( new var_str_t( block_content, fd.src_id, fd.idx ) );
		block_content.clear();
		goto begin_fetch;
	}

	if( line_ptr ) free( line_ptr );
	fseek( file, 0, SEEK_SET );

	// this should actually never occur since block_content
	// is always pushed back when end point is found
	// if( !block_content.empty() ) {
	// 	blocks.push_back( new var_str_t( block_content, fd.src_id, fd.idx ) );
	// }
	assert( block_content.empty() );

	return make< var_vec_t >( blocks, false );
}

var_base_t * fs_file_iterable_next( vm_state_t & vm, const fn_data_t & fd )
{
	var_file_iterable_t * it = FILE_ITERABLE( fd.args[ 0 ] );
	var_base_t * res = nullptr;
	if( !it->next( res ) ) return vm.nil;
	return res;
}

var_base_t * fs_walkdir( vm_state_t & vm, const fn_data_t & fd )
{
	std::vector< var_base_t * > v;
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected string argument for directory name, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( !fd.args[ 2 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected int argument for walk mode, found: %s",
			 vm.type_name( fd.args[ 2 ] ).c_str() );
		return nullptr;
	}
	if( !fd.args[ 3 ]->istype< var_str_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected string argument for file regex, found: %s",
			 vm.type_name( fd.args[ 2 ] ).c_str() );
		return nullptr;
	}
	std::string dir_str = STR( fd.args[ 1 ] )->get();
	size_t flags = mpz_get_ui( INT( fd.args[ 2 ] )->get() );
	std::string regex_str = STR( fd.args[ 3 ] )->get();
	std::regex regex( regex_str );
	if( dir_str.size() > 0 && dir_str.back() != '/' ) dir_str += "/";
	get_entries_internal( dir_str, v, flags, fd.src_id, fd.idx, regex );
	return make< var_vec_t >( v, false );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////// File Descriptor Functions ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// equivalent to open( path, O_WRONLY | O_CREAT | O_TRUNC, mode )
var_base_t * fs_fd_create( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected string argument for file name, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( !fd.args[ 2 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected int argument for mode, found: %s",
			 vm.type_name( fd.args[ 2 ] ).c_str() );
		return nullptr;
	}
	int res = creat( STR( fd.args[ 1 ] )->get().c_str(), mpz_get_si( INT( fd.args[ 2 ] )->get() ) );
	if( res < 0 ) {
		vm.fail( fd.src_id, fd.idx, "failed to create file: '%s', error: %s",
			 STR( fd.args[ 1 ] )->get().c_str(), strerror( errno ) );
		return nullptr;
	}
	return make< var_int_t >( res );
}

var_base_t * fs_fd_open( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected string argument for file name, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( !fd.args[ 2 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected int argument for open flags, found: %s",
			 vm.type_name( fd.args[ 2 ] ).c_str() );
		return nullptr;
	}
	int res = open( STR( fd.args[ 1 ] )->get().c_str(), mpz_get_si( INT( fd.args[ 2 ] )->get() ) );
	if( res < 0 ) {
		vm.fail( fd.src_id, fd.idx, "failed to open file: '%s', error: %s",
			 STR( fd.args[ 1 ] )->get().c_str(), strerror( errno ) );
		return nullptr;
	}
	return make< var_int_t >( res );
}

var_base_t * fs_fd_read( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected int argument for file descriptor, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( !fd.args[ 2 ]->istype< var_bytebuffer_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected bytebuffer containing data to write, found: %s",
			 vm.type_name( fd.args[ 2 ] ).c_str() );
		return nullptr;
	}
	var_bytebuffer_t * bb = BYTEBUFFER( fd.args[ 2 ] );
	errno = 0;
	ssize_t res = read( mpz_get_si( INT( fd.args[ 1 ] )->get() ), bb->get_buf(), bb->get_size() );
	if( res < 0 || errno != 0 ) {
		vm.fail( fd.src_id, fd.idx, "failed to read from file descriptor: '%d', error: %s",
			 mpz_get_si( INT( fd.args[ 2 ] )->get() ), strerror( errno ) );
		return nullptr;
	}
	return make< var_int_t >( res );
}

var_base_t * fs_fd_write( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected int argument for file descriptor, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( !fd.args[ 2 ]->istype< var_bytebuffer_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected bytebuffer containing data to write, found: %s",
			 vm.type_name( fd.args[ 2 ] ).c_str() );
		return nullptr;
	}
	var_bytebuffer_t * bb = BYTEBUFFER( fd.args[ 2 ] );
	errno = 0;
	ssize_t res = write( mpz_get_si( INT( fd.args[ 1 ] )->get() ), bb->get_buf(), bb->get_size() );
	if( res < 0 || errno != 0 ) {
		vm.fail( fd.src_id, fd.idx, "failed to write to file descriptor: '%d', error: %s",
			 mpz_get_si( INT( fd.args[ 2 ] )->get() ), strerror( errno ) );
		return nullptr;
	}
	return make< var_int_t >( res );
}

var_base_t * fs_fd_close( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected int argument for file descriptor, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	int res = close( mpz_get_si( INT( fd.args[ 1 ] )->get() ) );
	if( res < 0 ) {
		vm.fail( fd.src_id, fd.idx, "failed to close file descriptor: '%d', error: %s",
			 mpz_get_si( INT( fd.args[ 2 ] )->get() ), strerror( errno ) );
		return nullptr;
	}
	return make< var_int_t >( res );
}

INIT_MODULE( fs )
{
	var_src_t * src = vm.current_source();

	src->add_native_fn( "exists", fs_exists, 1 );
	src->add_native_fn( "fopen_native", fs_open, 2 );
	src->add_native_fn( "walkdir_native", fs_walkdir, 3 );

	vm.add_native_typefn< var_file_t >( "lines", fs_file_lines, 0, src_id, idx );
	vm.add_native_typefn< var_file_t >( "each_line", fs_file_each_line, 0, src_id, idx );
	vm.add_native_typefn< var_file_t >( "read_blocks", fs_file_read_blocks, 2, src_id, idx );
	vm.add_native_typefn< var_file_t >( "seek", fs_file_seek, 2, src_id, idx );

	vm.add_native_typefn< var_file_iterable_t >( "next", fs_file_iterable_next, 0, src_id, idx );

	// file descriptor
	src->add_native_fn( "open_native", fs_fd_open,  2 );
	src->add_native_fn( "read",	   fs_fd_read,  2 );
	src->add_native_fn( "write",	   fs_fd_write, 2 );
	src->add_native_fn( "close",	   fs_fd_close, 1 );

	// constants

	// fs.walkdir()
	src->add_native_var( "WALK_FILES", make_all< var_int_t >( WalkEntry::FILES, src_id, idx ) );
	src->add_native_var( "WALK_DIRS", make_all< var_int_t >( WalkEntry::DIRS, src_id, idx ) );
	src->add_native_var( "WALK_RECURSE", make_all< var_int_t >( WalkEntry::RECURSE, src_id, idx ) );

	// <file>.seek()
	src->add_native_var( "SEEK_SET", make_all< var_int_t >( SEEK_SET, src_id, idx ) );
	src->add_native_var( "SEEK_CUR", make_all< var_int_t >( SEEK_CUR, src_id, idx ) );
	src->add_native_var( "SEEK_END", make_all< var_int_t >( SEEK_END, src_id, idx ) );

	// file descriptor flags
	src->add_native_var( "O_RDONLY", make_all< var_int_t >( O_RDONLY, src_id, idx ) );
	src->add_native_var( "O_WRONLY", make_all< var_int_t >( O_WRONLY, src_id, idx ) );
	src->add_native_var( "O_RDWR", make_all< var_int_t >( O_RDWR, src_id, idx ) );
	src->add_native_var( "O_APPEND", make_all< var_int_t >( O_APPEND, src_id, idx ) );
	src->add_native_var( "O_CREAT", make_all< var_int_t >( O_CREAT, src_id, idx ) );
#if __linux__ || __APPLE__
	src->add_native_var( "O_DSYNC", make_all< var_int_t >( O_DSYNC, src_id, idx ) );
#endif
	src->add_native_var( "O_EXCL", make_all< var_int_t >( O_EXCL, src_id, idx ) );
	src->add_native_var( "O_NOCTTY", make_all< var_int_t >( O_NOCTTY, src_id, idx ) );
	src->add_native_var( "O_NONBLOCK", make_all< var_int_t >( O_NONBLOCK, src_id, idx ) );
#ifdef __linux__
	src->add_native_var( "O_RSYNC", make_all< var_int_t >( O_RSYNC, src_id, idx ) );
#endif
	src->add_native_var( "O_SYNC", make_all< var_int_t >( O_SYNC, src_id, idx ) );
	src->add_native_var( "O_TRUNC", make_all< var_int_t >( O_TRUNC, src_id, idx ) );
	return true;
}


void get_entries_internal( const std::string & dir_str, std::vector< var_base_t * > & v,
			   const size_t & flags, const int src_id, const int idx,
			   const std::regex & regex )
{
	DIR * dir;
	struct dirent * ent;
	if( ( dir = opendir( dir_str.c_str() ) ) == NULL ) return;

	while( ( ent = readdir( dir ) ) != NULL ) {
		if( strcmp( ent->d_name, "." ) == 0 || strcmp( ent->d_name, ".." ) == 0 ) continue;
		std::string entry = dir_str + ent->d_name;
		if( ( !( flags & WalkEntry::RECURSE ) || ent->d_type != DT_DIR ) && !std::regex_match( entry, regex ) ) {
			continue;
		}
		if( ent->d_type == DT_DIR ) {
			if( flags & WalkEntry::RECURSE ) {
				get_entries_internal( entry + "/", v, flags, src_id, idx, regex );
			} else if( flags & WalkEntry::DIRS ) {
				v.push_back( new var_str_t( entry, src_id, idx ) );
			}
			continue;
		}
		if( flags & WalkEntry::FILES || flags & WalkEntry::RECURSE ) {
			v.push_back( new var_str_t( entry, src_id, idx ) );
		}
	}
	closedir( dir );
}