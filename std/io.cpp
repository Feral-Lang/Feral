/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "../src/VM/VM.hpp"

#include "../include/std/fs_type.hpp"

const size_t MAX_C_STR_LEN = 1025;
extern std::unordered_map< std::string, const char * > COL;
int apply_colors( std::string & str );

var_base_t * print( vm_state_t & vm, const fn_data_t & fd )
{
	for( size_t i = 1; i < fd.args.size(); ++i ) {
		std::string str;
		if( !fd.args[ i ]->to_str( vm, str, fd.src_id, fd.idx ) ) {
			return nullptr;
		}
		fprintf( stdout, "%s", str.c_str() );
	}
	return vm.nil;
}

var_base_t * println( vm_state_t & vm, const fn_data_t & fd )
{
	for( size_t i = 1; i < fd.args.size(); ++i ) {
		std::string str;
		if( !fd.args[ i ]->to_str( vm, str, fd.src_id, fd.idx ) ) {
			return nullptr;
		}
		fprintf( stdout, "%s", str.c_str() );
	}
	fprintf( stdout, "\n" );
	return vm.nil;
}

var_base_t * fprint( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_file_t >() ) {
		vm.fail( fd.args[ 1 ]->src_id(), fd.args[ 1 ]->idx(), "expected a file argument for fflush, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( FILE( fd.args[ 1 ] )->get() == nullptr ) {
		vm.fail( fd.args[ 1 ]->src_id(), fd.args[ 1 ]->idx(), "file has probably been closed already",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	for( size_t i = 2; i < fd.args.size(); ++i ) {
		std::string str;
		if( !fd.args[ i ]->to_str( vm, str, fd.src_id, fd.idx ) ) {
			return nullptr;
		}
		fprintf( FILE( fd.args[ 1 ] )->get(), "%s", str.c_str() );
	}
	return vm.nil;
}

var_base_t * fprintln( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_file_t >() ) {
		vm.fail( fd.args[ 1 ]->src_id(), fd.args[ 1 ]->idx(), "expected a file argument for fflush, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( FILE( fd.args[ 1 ] )->get() == nullptr ) {
		vm.fail( fd.args[ 1 ]->src_id(), fd.args[ 1 ]->idx(), "file has probably been closed already",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	for( size_t i = 2; i < fd.args.size(); ++i ) {
		std::string str;
		if( !fd.args[ i ]->to_str( vm, str, fd.src_id, fd.idx ) ) {
			return nullptr;
		}
		fprintf( FILE( fd.args[ 1 ] )->get(), "%s", str.c_str() );
	}
	fprintf( FILE( fd.args[ 1 ] )->get(), "\n" );
	return vm.nil;
}

var_base_t * col_print( vm_state_t & vm, const fn_data_t & fd )
{
	for( size_t i = 1; i < fd.args.size(); ++i ) {
		std::string str;
		if( !fd.args[ i ]->to_str( vm, str, fd.src_id, fd.idx ) ) {
			return nullptr;
		}
		apply_colors( str );
		fprintf( stdout, "%s", str.c_str() );
	}
	return vm.nil;
}

var_base_t * col_println( vm_state_t & vm, const fn_data_t & fd )
{
	for( size_t i = 1; i < fd.args.size(); ++i ) {
		std::string str;
		if( !fd.args[ i ]->to_str( vm, str, fd.src_id, fd.idx ) ) {
			return nullptr;
		}
		apply_colors( str );
		fprintf( stdout, "%s", str.c_str() );
	}
	fprintf( stdout, "\n" );
	return vm.nil;
}

var_base_t * col_dprint( vm_state_t & vm, const fn_data_t & fd )
{
	for( size_t i = 1; i < fd.args.size(); ++i ) {
		std::string str;
		if( !fd.args[ i ]->to_str( vm, str, fd.src_id, fd.idx ) ) {
			return nullptr;
		}
		apply_colors( str );
		fprintf( stderr, "%s", str.c_str() );
	}
	return vm.nil;
}

var_base_t * col_dprintln( vm_state_t & vm, const fn_data_t & fd )
{
	for( size_t i = 1; i < fd.args.size(); ++i ) {
		std::string str;
		if( !fd.args[ i ]->to_str( vm, str, fd.src_id, fd.idx ) ) {
			return nullptr;
		}
		apply_colors( str );
		fprintf( stderr, "%s", str.c_str() );
	}
	fprintf( stderr, "\n" );
	return vm.nil;
}

var_base_t * scan( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.args[ 1 ]->src_id(), fd.args[ 1 ]->idx(), "expected string data for input prompt, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	fprintf( stdout, "%s", STR( fd.args[ 1 ] )->get().c_str() );

	char str[ MAX_C_STR_LEN ];
	fgets( str, MAX_C_STR_LEN, stdin );
	std::string res( str );

	if( res.back() == '\r' ) res.pop_back();
	if( res.back() == '\n' ) res.pop_back();

	return make< var_str_t >( res );
}

var_base_t * scaneof( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_str_t >() ) {
		vm.fail( fd.args[ 1 ]->src_id(), fd.args[ 1 ]->idx(), "expected string data for input prompt, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	fprintf( stdout, "%s", STR( fd.args[ 1 ] )->get().c_str() );

	std::string line, res;

	while( std::getline( std::cin, line ) ) res += line;

	if( res.back() == '\r' ) res.pop_back();
	if( res.back() == '\n' ) res.pop_back();

	return make< var_str_t >( res );
}

var_base_t * fflush( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_file_t >() ) {
		vm.fail( fd.args[ 1 ]->src_id(), fd.args[ 1 ]->idx(), "expected a file argument for fflush, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( FILE( fd.args[ 1 ] )->get() == nullptr ) {
		vm.fail( fd.args[ 1 ]->src_id(), fd.args[ 1 ]->idx(), "file has probably been closed already",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	fflush( FILE( fd.args[ 1 ] )->get() );
	return vm.nil;
}

INIT_MODULE( io )
{
	var_src_t * src = vm.current_source();

	src->add_native_fn( "print", print, 1, true );
	src->add_native_fn( "println", println, 0, true );
	src->add_native_fn( "fprint", fprint, 2, true );
	src->add_native_fn( "fprintln", fprintln, 1, true );
	src->add_native_fn( "cprint", col_print, 1, true );
	src->add_native_fn( "cprintln", col_println, 0, true );
	src->add_native_fn( "cdprint", col_dprint, 1, true );
	src->add_native_fn( "cdprintln", col_dprintln, 0, true );
	src->add_native_fn( "scan_native", scan, 1 );
	src->add_native_fn( "scaneof_native", scaneof, 1 );
	src->add_native_fn( "fflush", fflush, 1 );

	// stdout and stderr cannot be owned by a var_file_t
	src->add_native_var( "stdout", make_all< var_file_t >( stdout, "w", src_id, idx, false ) );
	src->add_native_var( "stderr", make_all< var_file_t >( stderr, "w", src_id, idx, false ) );
	return true;
}

std::unordered_map< std::string, const char * > COL = {
	{ "0", "\033[0m" },

	{ "r", "\033[0;31m" },
	{ "g", "\033[0;32m" },
	{ "y", "\033[0;33m" },
	{ "b", "\033[0;34m" },
	{ "m", "\033[0;35m" },
	{ "c", "\033[0;36m" },
	{ "w", "\033[0;37m" },

	{ "br", "\033[1;31m" },
	{ "bg", "\033[1;32m" },
	{ "by", "\033[1;33m" },
	{ "bb", "\033[1;34m" },
	{ "bm", "\033[1;35m" },
	{ "bc", "\033[1;36m" },
	{ "bw", "\033[1;37m" },
};

int apply_colors( std::string & str )
{
	int chars = 0;
	for( size_t i = 0; i < str.size(); ) {
		if( str[ i ] == '{' && ( i == 0 || ( str[ i - 1 ] != '$' && str[ i - 1 ] != '%' && str[ i - 1 ] != '#' && str[ i - 1 ] != '\\' ) ) ) {
			str.erase( str.begin() + i );
			if( i < str.size() && str[ i ] == '{' ) {
				++i;
				++chars;
				continue;
			}

			std::string var;

			while( i < str.size() && str[ i ] != '}' ) {
				var += str[ i ];
				str.erase( str.begin() + i );
			}

			// Remove the ending brace
			if( i < str.size() ) str.erase( str.begin() + i );

			if( var.empty() ) continue;

			if( COL.find( var ) != COL.end() ) {
				std::string val = COL[ var ];
				str.insert( str.begin() + i, val.begin(), val.end() );
				i += val.size();
			}
		}
		else {
			++i;
			++chars;
		}
	}
	return chars;
}