/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <fstream>

#include "../Common/FS.hpp"
#include "../VM/VM.hpp"

#include "Config.hpp"
#include "Args.hpp"
#include "Lex.hpp"
#include "Parser.hpp"
#include "CodeGen.hpp"

#include "LoadFile.hpp"

srcfile_t * fmod_load( const std::string & src_file, const size_t & flags, const bool is_main_src, Errors & err )
{
	std::string src_dir;
	std::string src_path = fs::abs_path( src_file, & src_dir );

	srcfile_t * src = vm::src_new( src_dir, src_path, is_main_src );

	// lexical analysis
	lex::toks_t toks;

	phelper_t ph( * src, toks );
	ptree_t * ptree = nullptr;
	bcode_t & bc = src->bcode();

	err = src->load_file();
	if( err != E_OK ) goto fail;

	err = lex::tokenize( * src, toks );
	if( err != E_OK ) goto fail;

	// show tokens
	if( flags & OPT_T ) {
		fprintf( stdout, "Tokens (%zu):\n", toks.size() );
		for( size_t i = 0; i < toks.size(); ++i ) {
			auto & tok = toks[ i ];
			fprintf( stdout, "ID: %zu\tIdx: %zu\tType: %s\tSymbol: %s\n",
				 i, tok.pos, TokStrs[ tok.type ], tok.data.c_str() );
		}
	}

	err = parser::parse( * src, toks, ptree, ph );
	if( err != E_OK ) goto fail;

	// show tree
	if( flags & OPT_P ) {
		fprintf( stdout, "Parse Tree:\n" );
		for( auto it = ptree->stmts().begin(); it != ptree->stmts().end(); ++it ) {
			( * it )->disp( it != ptree->stmts().end() - 1 );
		}
	}

	err = gen::generate( ptree, bc ) ? E_OK : E_CODEGEN_FAIL;
	if( err != E_OK ) goto fail;

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
		std::string comp_file = src_file + ".ferc";
		std::ofstream f( comp_file, std::ios::out | std::ios::binary );
		if( !f.good() ) {
			fprintf( stderr, "failed to open file %s for writing bytecode\n", comp_file.c_str() );
			err = E_FAIL;
			goto fail;
		}
		if( bc.size() == 0 ) {
			fprintf( stderr, "nothing to write - file empty\n" );
			f.close();
			err = E_FAIL;
			goto fail;
		}
		fprintf( stdout, "byte compiling %s -> %s ...\n",
			 src_file.c_str(), comp_file.c_str() );
		f.write( ( char * ) ( & bc.bcode()[ 0 ] ), sizeof( op_t ) * bc.size() );
		f.close();
	}
	delete ptree;
	return src;
fail:
	if( ptree ) delete ptree;
	delete src;
	return nullptr;
}
