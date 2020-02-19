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
	std::unordered_map< std::string, std::string > args;
	std::vector< std::string > code_args;
	size_t flags = args::parse( argc, ( const char ** )argv, args, code_args );

	if( flags & OPT_V ) {
		fprintf( stdout, "Feral %d.%d.%d\nBuilt with %s\nOn %s\n", FERAL_VERSION_MAJOR,
			 FERAL_VERSION_MINOR, FERAL_VERSION_PATCH, BUILD_CXX_COMPILER, BUILD_DATE );
		return E_OK;
	}

	if( args.find( "__main__" ) == args.end() ) {
		fprintf( stderr, "usage: %s [flags] <source file>\n", argv[ 0 ] );
		return E_FAIL;
	}

	int err = E_OK;

	// feral binary absolute location
	std::string fer_bin = fs::abs_path( argv[ 0 ] );

	std::string src_dir;
	std::string src_path = fs::abs_path( args[ "__main__" ], & src_dir );

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

	err = gen::generate( ptree, bc ) ? E_OK : E_CODEGEN_FAIL;
	if( err != E_OK ) goto cleanup;

	// show bytecode
	if( flags & OPT_B ) {
		fprintf( stdout, "Byte Code (%zu):\n", bc.size() );
		const std::vector< op_t > & bcode = bc.bcode();
		int id_padding = std::to_string( bcode.size() ).size();
		for( size_t i = 0; i < bcode.size(); ++i ) {
			fprintf( stdout, "ID: %-*zu  %*s ", id_padding, i, 12, OpCodeStrs[ bcode[ i ].op ] );
			if( bcode[ i ].dtype == ODT_BOOL ) {
				fprintf( stdout, "[%s]\t[BOOL]\n", bcode[ i ].data.b ? "yes" : "no" );
			} else if( bcode[ i ].dtype == ODT_SZ ) {
				if( bcode[ i ].op == OP_BINARY ) {
					fprintf( stdout, "[%s]\t[SZ]\n", OpBinaryStrs[ bcode[ i ].data.sz ] );
				} else if( bcode[ i ].op == OP_UNARY ) {
					fprintf( stdout, "[%s]\t[SZ]\n", OpUnaryStrs[ bcode[ i ].data.sz ] );
				} else if( bcode[ i ].op == OP_COMP ) {
					fprintf( stdout, "[%s]\t[SZ]\n", OpCompStrs[ bcode[ i ].data.sz ] );
				} else {
					fprintf( stdout, "[%zu]\t[SZ]\n", bcode[ i ].data.sz );
				}
			} else {
				fprintf( stdout, "[%s]\t[%s]\n", bcode[ i ].data.s,
					 OpDataTypeStrs[ bcode[ i ].dtype ] );
			}
		}
	}

	if( flags & OPT_C ) {
		if( args.find( "c" ) == args.end() ) {
			fprintf( stderr, "no file name provided for bytecompile output after -c flag\n" );
			err = E_FAIL;
			goto cleanup;
		}
		std::ofstream f( args[ "c" ], std::ios::out | std::ios::binary );
		if( !f.good() ) {
			fprintf( stderr, "failed to open file %s for writing bytecode\n", args[ "c" ].c_str() );
			err = E_FAIL;
			goto cleanup;
		}
		if( bc.size() == 0 ) {
			fprintf( stderr, "nothing to write - file empty\n" );
			f.close();
			err = E_FAIL;
			goto cleanup;
		}
		fprintf( stdout, "byte compiling %s -> %s ...\n",
			 args[ "__main__" ].c_str(), args[ "c" ].c_str() );
		f.write( ( char * ) ( & bc.bcode()[ 0 ] ), sizeof( op_t ) * bc.size() );
		f.close();
	}

	if( !( flags & OPT_D ) ) {
		err = vm::exec( main_src, bc );
	}
cleanup:
	delete ptree;
	return err;
}
