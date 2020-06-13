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

#include <unistd.h>

#include "VM/VM.hpp"

#include "std/term_type.hpp"

static struct termios mode_orig;

var_base_t * term_get_attrs( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected file descriptor id for get_attrs, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	struct termios term;
	tcgetattr( mpz_get_si( INT( fd.args[ 1 ] )->get() ), & term );
	return make< var_term_t >( term );
}

var_base_t * term_set_attrs( vm_state_t & vm, const fn_data_t & fd )
{
	if( !fd.args[ 1 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected file descriptor id for set_attrs, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	if( !fd.args[ 2 ]->istype< var_term_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected file descriptor id for set_attrs, found: %s",
			 vm.type_name( fd.args[ 1 ] ).c_str() );
		return nullptr;
	}
	bool done = tcsetattr( mpz_get_si( INT( fd.args[ 1 ] )->get() ), TCSAFLUSH, & TERM( fd.args[ 2 ] )->get() ) != -1;
	return make< var_bool_t >( done );
}

INIT_MODULE( term )
{
	var_src_t * src = vm.current_source();

	tcgetattr( STDIN_FILENO, & mode_orig );

	struct termios mode_raw = mode_orig;
	mode_raw.c_iflag &= ~( BRKINT | ICRNL | INPCK | ISTRIP | IXON );
	mode_raw.c_oflag &= ~( OPOST );
	mode_raw.c_cflag |= ( CS8 );
	mode_raw.c_lflag &= ~( ECHO | ICANON | IEXTEN | ISIG );
	mode_raw.c_cc[ VMIN ] = 1;
	mode_raw.c_cc[ VTIME ] = 0;
	src->add_native_var( "mode_orig", make_all< var_term_t >( mode_orig, src_id, idx ) );
	src->add_native_var( "mode_raw", make_all< var_term_t >( mode_raw, src_id, idx ) );

	src->add_native_fn( "get_mode", term_get_attrs, 1 );
	src->add_native_fn( "set_mode", term_set_attrs, 2 );

	src->add_native_var( "fd_stdin",  make_all< var_int_t >( STDIN_FILENO,  src_id, idx ) );
	src->add_native_var( "fd_stdout", make_all< var_int_t >( STDOUT_FILENO, src_id, idx ) );
	src->add_native_var( "fd_stderr", make_all< var_int_t >( STDERR_FILENO, src_id, idx ) );

	return true;
}