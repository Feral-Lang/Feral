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

#include <fstream>
#include <cstdio>
#include <cstring>

#include "Common/Config.hpp"
#include "Common/FS.hpp"
#include "Common/Env.hpp"
#include "Common/String.hpp"

#include "Compiler/Args.hpp"
#include "Compiler/LoadFile.hpp"

#include "VM/VM.hpp"

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
	std::string feral_base, feral_bin;
	feral_bin = fs::abs_path( env::get_proc_path(), & feral_base, true );

	std::string src_file = args[ "__main__" ];
	if( src_file.empty() ) src_file = "repl";

	if( ( src_file == "testdir" || src_file == "repl" ) && !fs::exists( src_file ) ) {
		src_file = feral_base + "/include/feral/" + src_file + fmod_ext();
	} else if( src_file == "install" || src_file == "build" ) {
		if( src_file == "install" ) code_args.insert( code_args.begin(), "install" );
		src_file = std::string( "build" ) + fmod_ext();
	}

	if( src_file.empty() || flags & OPT_H ) {
		fprintf( stderr, "usage: %s [flags] <source file>\n", argv[ 0 ] );
		return E_FAIL;
	}

	if( !fs::exists( src_file ) ) {
		fprintf( stderr, "file load fail: file '%s' does not exist\n", src_file.c_str() );
		err = E_FAIL;
		return err;
	}

	std::string src_dir;
	src_file = fs::abs_path( src_file, & src_dir );

	srcfile_t * main_src = fmod_load( src_file, src_dir, flags, true, err );
	if( err != E_OK ) {
		return err;
	}

	int exec_err = 0;
	if( !( flags & OPT_D ) ) {
		vm_state_t vm( feral_bin, feral_base, code_args, flags );
		vm.set_fmod_load_fn( fmod_load );
		vm.set_fmod_read_code_fn( fmod_read_code );
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
