/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <cstdio>
#include <cstring>

#include "../Common/Errors.hpp"
#include "../Common/FS.hpp"
#include "../VM/VM.hpp"

#include "Config.hpp"
#include "Args.hpp"
#include "Lex.hpp"
#include "Parser.hpp"
#include "CodeGen.hpp"

int main( int argc, char ** argv )
{
	std::vector< std::string > args;
	size_t flags = args::parse( argc, ( const char ** )argv, args );

	if( flags & OPT_V ) {
		fprintf( stdout, "Feral %d.%d.%d\nBuilt with %s\nOn %s\n", FERAL_VERSION_MAJOR,
			 FERAL_VERSION_MINOR, FERAL_VERSION_PATCH, BUILD_CXX_COMPILER, BUILD_DATE );
		return E_OK;
	}

	if( args.size() < 1 ) {
		fprintf( stderr, "usage: %s [flags] <source file>\n", argv[ 0 ] );
		return E_FAIL;
	}

	Errors err = E_OK;

	// feral binary absolute location
	std::string fer_bin = fs::abs_path( argv[ 0 ] );

	std::string src_dir;
	std::string src_path = fs::abs_path( args[ 0 ], & src_dir );

	srcfile_t main_src = vm::src_new( src_dir, src_path, true );
	err = main_src.load_file();
	if( err != E_OK ) return err;

	// lexical analysis
	lex::toks_t toks;
	err = lex::tokenize( main_src, toks );
	if( err != E_OK ) return err;

	// show tokens
	if( flags & OPT_T ) {
		fprintf( stdout, "Tokens (%zu):\n", toks.size() );
		for( size_t i = 0; i < toks.size(); ++i ) {
			auto & tok = toks[ i ];
			fprintf( stdout, "ID: %zu\tIdx: %zu\tType: %s\tSymbol: %s\n",
				 i, tok.pos, TokStrs[ tok.type ], tok.data.c_str() );
		}
	}

	phelper_t ph( main_src, toks );
	ptree_t * ptree = nullptr;

	bcode_t bc;

	err = parser::parse( main_src, toks, ptree, ph );
	if( err != E_OK ) goto cleanup;

	// show tree
	if( flags & OPT_P ) {
		fprintf( stdout, "Parse Tree:\n" );
		for( auto it = ptree->stmts().begin(); it != ptree->stmts().end(); ++it ) {
			( * it )->disp( it != ptree->stmts().end() - 1 );
		}
	}

	err = gen::generate( ptree, bc );
	if( err != E_OK ) goto cleanup;

	// show bytecode
	if( flags & OPT_B ) {
		fprintf( stdout, "Byte Code (%zu):\n", bc.size() );
		const std::vector< op_t > & bcode = bc.bcode();
		for( size_t i = 0; i < bcode.size(); ++i ) {
			fprintf( stdout, "ID: %zu\tInstruction: %s\n",
				 i, OpCodeStrs[ bcode[ i ].op ] );
		}
	}
cleanup:
	delete ptree;
	return err;
}
