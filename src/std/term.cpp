/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
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
	tcgetattr( INT( fd.args[ 1 ])->get().get_si(), & term );
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
	bool done = tcsetattr( INT( fd.args[ 1 ])->get().get_si(), TCSAFLUSH, & TERM( fd.args[ 2 ] )->get() ) != -1;
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
	mode_raw.c_cc[ VMIN ] = 0;
	mode_raw.c_cc[ VTIME ] = 1;
	src->add_native_var( "mode_orig", make_all< var_term_t >( mode_orig, src_id, idx ) );
	src->add_native_var( "mode_raw", make_all< var_term_t >( mode_raw, src_id, idx ) );

	src->add_native_fn( "get_mode", term_get_attrs, 1 );
	src->add_native_fn( "set_mode", term_set_attrs, 2 );

	src->add_native_var( "fd_stdin",  make_all< var_int_t >( STDIN_FILENO,  src_id, idx ) );
	src->add_native_var( "fd_stdout", make_all< var_int_t >( STDOUT_FILENO, src_id, idx ) );
	src->add_native_var( "fd_stderr", make_all< var_int_t >( STDERR_FILENO, src_id, idx ) );

	return true;
}