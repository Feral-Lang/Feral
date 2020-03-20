/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <fstream>
#include <cstdio>
#include <cstring>

#include "../Common/String.hpp"
#include "../Common/FS.hpp"
#include "../VM/VM.hpp"

#include "Config.hpp"
#include "Args.hpp"
#include "LoadFile.hpp"

int main( int argc, char ** argv )
{
	std::unordered_map< std::string, std::string > args;
	std::vector< std::string > code_args;
	size_t flags = args::parse( argc, ( const char ** )argv, args, code_args );

	if( flags & OPT_V ) {
		fprintf( stdout, "Feral %d.%d.%d\nBuilt with %s\nOn %s\n", FERAL_VERSION_MAJOR,
			 FERAL_VERSION_MINOR, FERAL_VERSION_PATCH, BUILD_CXX_COMPILER, BUILD_DATE );
		return E_OK;
	}

	Errors err = E_OK;

	// feral binary absolute location
	std::string fer_bin = argv[ 0 ];

	std::string src_file = args[ "__main__" ];

	if( src_file == "init" ) {
		if( fs::exists( src_file + fmod_ext() ) ) {
			src_file += fmod_ext();
		} else {
			src_file = STRINGIFY( BUILD_PREFIX_DIR ) "/include/feral/" + src_file + fmod_ext();
		}
	} else if( src_file == "install" || src_file == "build" ) {
		if( src_file == "install" ) code_args.insert( code_args.begin(), "install" );
		src_file = std::string( "build" ) + fmod_ext();
	}

	if( src_file.empty() ) {
		fprintf( stderr, "usage: %s [flags] <source file>\n", argv[ 0 ] );
		return E_FAIL;
	}

	if( src_file.find( fmod_ext() ) == std::string::npos ) {
		src_file += fmod_ext();
	}

	srcfile_t * main_src = fmod_load( src_file, flags, true, err );
	if( err != E_OK ) {
		if( main_src ) delete main_src;
		return err;
	}

	int exec_err = 0;
	if( !( flags & OPT_D ) ) {
		vm_state_t vm( fer_bin, code_args, flags );
		vm.set_fmod_load_fn( fmod_load );
		vm.push_src( main_src, 0 );
		if( !vm.load_core_mods() ) {
			vm.pop_src();
			err = E_EXEC_FAIL;
			return err;
		}
		exec_err = vm::exec( vm );
		vm.pop_src();
	} else {
		if( main_src ) delete main_src;
	}
	return exec_err;
}
