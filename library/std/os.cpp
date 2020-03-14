/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <chrono>
#include <thread>
#include <cstdlib>
#include <sys/wait.h>

#include "../../src/VM/VM.hpp"

int exec_internal( const std::string & file );
std::string dir_part( const std::string & full_loc );

var_base_t * sleep_custom( vm_state_t & vm, const fn_data_t & fd )
{
	if( fd.args[ 1 ]->type() != VT_INT ) {
		vm.src_stack.back()->src()->fail( fd.idx, "expected integer argument for sleep time, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	std::this_thread::sleep_for(
		std::chrono::milliseconds( INT( fd.args[ 1 ] )->get().get_ui() )
	);
	return vm.nil;
}

var_base_t * get_env( vm_state_t & vm, const fn_data_t & fd )
{
	if( fd.args[ 1 ]->type() != VT_STR ) {
		vm.src_stack.back()->src()->fail( fd.idx, "expected string argument for env variable name, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	std::string var = STR( fd.args[ 1 ] )->get();
	const char * env = getenv( var.c_str() );
	return make< var_str_t >( env == NULL ? "" : env );
}

var_base_t * set_env( vm_state_t & vm, const fn_data_t & fd )
{
	if( fd.args[ 1 ]->type() != VT_STR ) {
		vm.src_stack.back()->src()->fail( fd.idx, "expected string argument for env variable name, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	if( fd.args[ 2 ]->type() != VT_STR ) {
		vm.src_stack.back()->src()->fail( fd.idx, "expected string argument for env variable value, found: %s",
						  vm.type_name( fd.args[ 2 ]->type() ).c_str() );
		return nullptr;
	}
	if( fd.args.size() > 3 && fd.args[ 3 ]->type() != VT_BOOL ) {
		vm.src_stack.back()->src()->fail( fd.idx, "expected boolean argument for overwrite existing env variable, found: %s",
						  vm.type_name( fd.args[ 3 ]->type() ).c_str() );
		return nullptr;
	}
	std::string var = STR( fd.args[ 1 ] )->get();
	std::string val = STR( fd.args[ 2 ] )->get();

	bool overwrite = false;
	if( fd.args.size() > 3 ) {
		overwrite = BOOL( fd.args[ 3 ] )->get();
	}
	return make< var_int_t >( setenv( var.c_str(), val.c_str(), overwrite ) );
}

var_base_t * exec_custom( vm_state_t & vm, const fn_data_t & fd )
{
	if( fd.args[ 1 ]->type() != VT_STR ) {
		vm.src_stack.back()->src()->fail( fd.idx, "expected string argument for command, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	std::string cmd = STR( fd.args[ 1 ] )->get();

	FILE * pipe = popen( cmd.c_str(), "r" );
	if( !pipe ) return make< var_int_t >( 1 );
	char * line = NULL;
	size_t len = 0;
	ssize_t nread;

	while( ( nread = getline( & line, & len, pipe ) ) != -1 ) {
		fprintf( stdout, "%s", line );
	}
	free( line );
	int res = pclose( pipe );

	res = WEXITSTATUS( res );
	return make< var_int_t >( res );
}

var_base_t * install( vm_state_t & vm, const fn_data_t & fd )
{
	if( fd.args[ 1 ]->type() != VT_STR ) {
		vm.src_stack.back()->src()->fail( fd.idx, "expected string argument for source, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	if( fd.args[ 2 ]->type() != VT_STR ) {
		vm.src_stack.back()->src()->fail( fd.idx, "expected string argument for destination, found: %s",
						  vm.type_name( fd.args[ 2 ]->type() ).c_str() );
		return nullptr;
	}
	std::string src = STR( fd.args[ 1 ] )->get(),
		    dest = STR( fd.args[ 2 ] )->get();

	if( src.empty() || dest.empty() ) {
		return make< var_int_t >( 0 );
	}

	if( exec_internal( "mkdir -p " + dest ) != 0 ) {
		return make< var_int_t >( -1 );
	}
	std::string cmd_str;
#if __linux__ || __ANDROID__
	cmd_str = "cp -r --remove-destination " + src + " " + dest;
#elif __APPLE__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__ || __bsdi__ || __DragonFly__
	cmd_str = "cp -rf " + src + " " + dest;
#endif
	return make< var_int_t >( exec_internal( cmd_str ) );
}

var_base_t * os_get_name( vm_state_t & vm, const fn_data_t & fd )
{
	std::string os_str;
#if __ANDROID__
	os_str = "android";
#elif __linux__
	os_str = "linux";
#elif __APPLE__
	os_str = "macos";
#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__ || __bsdi__ || __DragonFly__
	os_str = "bsd";
#endif
	return make< var_str_t >( os_str );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////// Extra Functions ///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_base_t * os_mkdir( vm_state_t & vm, const fn_data_t & fd )
{
	if( fd.args[ 1 ]->type() != VT_STR ) {
		vm.src_stack.back()->src()->fail( fd.idx, "expected string argument for destination directory, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	std::string dest = STR( fd.args[ 1 ] )->get();

	for( size_t i = 2; i < fd.args.size(); ++i ) {
		if( fd.args[ i ]->type() != VT_STR ) {
			vm.src_stack.back()->src()->fail( fd.idx, "expected string argument for destination directory, found: %s",
							  vm.type_name( fd.args[ i ]->type() ).c_str() );
			return nullptr;
		}
		std::string tmpdest = STR( fd.args[ i ] )->get();
		if( tmpdest.empty() ) continue;
		dest += " " + tmpdest;
	}
	return make< var_int_t >( exec_internal( "mkdir -p " + dest ) );
}

var_base_t * os_rm( vm_state_t & vm, const fn_data_t & fd )
{
	if( fd.args[ 1 ]->type() != VT_STR ) {
		vm.src_stack.back()->src()->fail( fd.idx, "expected string argument for destination directory, found: %s",
						  vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}
	std::string dest = STR( fd.args[ 1 ] )->get();

	for( size_t i = 2; i < fd.args.size(); ++i ) {
		if( fd.args[ i ]->type() != VT_STR ) {
			vm.src_stack.back()->src()->fail( fd.idx, "expected string argument for destination directory, found: %s",
							  vm.type_name( fd.args[ i ]->type() ).c_str() );
			return nullptr;
		}
		std::string tmpdest = STR( fd.args[ i ] )->get();
		if( tmpdest.empty() ) continue;
		dest += " " + tmpdest;
	}
	if( dest.empty() ) {
		return make< var_int_t >( 0 );
	}
	return make< var_int_t >( exec_internal( "rm -r " + dest ) );
}

REGISTER_MODULE( os )
{
	var_src_t * src = vm.src_stack.back();
	const std::string & src_name = src->src()->path();

	src->add_nativefn( "sleep", sleep_custom, { "" } );

	src->add_nativefn( "get_env", get_env, { "" } );
	src->add_nativefn( "set_env", set_env, { "", "" }, {}, true );

	src->add_nativefn( "exec", exec_custom, { "" } );
	src->add_nativefn( "install", install, { "", "" } );

	src->add_nativefn( "os_get_name_native", os_get_name );

	src->add_nativefn( "mkdir", os_mkdir, { "" }, {}, true );
	src->add_nativefn( "rm", os_rm, { "" }, {}, true );

	return true;
}

int exec_internal( const std::string & cmd )
{
	FILE * pipe = popen( cmd.c_str(), "r" );
	if( !pipe ) return 1;
	char * line = NULL;
	size_t len = 0;
	ssize_t nread;

	while( ( nread = getline( & line, & len, pipe ) ) != -1 );
	free( line );
	int res = pclose( pipe );
	return WEXITSTATUS( res );
}

std::string dir_part( const std::string & full_loc )
{
	auto loc = full_loc.find_last_of( '/' );
	if( loc == std::string::npos ) return ".";
	if( loc == 0 ) return "/";
	return full_loc.substr( 0, loc );
}