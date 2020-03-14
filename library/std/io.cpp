/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include "../../src/VM/VM.hpp"

var_base_t * println( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.src_stack.back()->src();
	for( size_t i = 1; i < fd.args.size(); ++i ) {
		auto & arg = fd.args[ i ];
		var_fn_t * str_fn = vm.get_typefn( arg->type(), "str" );
		if( !str_fn ) str_fn = vm.get_typefn( VT_ALL, "str" );
		if( !str_fn ) {
			src->fail( arg->idx(), "no 'str' function implement for type: '%zu' or global type", arg->type() );
			return nullptr;
		}
		if( !FN( str_fn )->call( vm, { arg }, {}, src->id(), fd.idx ) ) {
			src->fail( arg->idx(), "function call 'str' for type: %zu failed", arg->type() );
			return nullptr;
		}
		var_base_t * str = vm.vm_stack->pop( false );
		if( str->type() != VT_STR ) {
			src->fail( arg->idx(), "expected string return type from 'str' function, received: %s", vm.type_name( str->type() ).c_str() );
			var_dref( str );
			return nullptr;
		}
		fprintf( stdout, "%s", STR( str )->get().c_str() );
		if( i < fd.args.size() - 1 ) fprintf( stdout, " " );
		var_dref( str );
	}
	fprintf( stdout, "\n" );
	return vm.nil;
}

static std::unordered_map< std::string, const char * > COL = {
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

var_base_t * col_println( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src = vm.src_stack.back()->src();
	for( size_t i = 1; i < fd.args.size(); ++i ) {
		auto & arg = fd.args[ i ];
		var_fn_t * str_fn = vm.get_typefn( arg->type(), "str" );
		if( !str_fn ) str_fn = vm.get_typefn( VT_ALL, "str" );
		if( !str_fn ) {
			src->fail( arg->idx(), "no 'str' function implement for type: '%zu' or global type", arg->type() );
			return nullptr;
		}
		if( !FN( str_fn )->call( vm, { arg }, {}, src->id(), fd.idx ) ) {
			src->fail( arg->idx(), "function call 'str' for type: %zu failed", arg->type() );
			return nullptr;
		}
		var_base_t * str = vm.vm_stack->pop( false );
		if( str->type() != VT_STR ) {
			src->fail( arg->idx(), "expected string return type from 'str' function, received: %s", vm.type_name( str->type() ).c_str() );
			var_dref( str );
			return nullptr;
		}
		std::string data = STR( str )->get();
		apply_colors( data );
		fprintf( stdout, "%s", data.c_str() );
		if( i < fd.args.size() - 1 ) fprintf( stdout, " " );
		var_dref( str );
	}
	fprintf( stdout, "\n" );
	return vm.nil;
}

REGISTER_MODULE( io )
{
	var_src_t * src = vm.src_stack.back();
	const std::string & src_name = src->src()->path();
	src->add_nativefn( "println", println, {}, {}, true );
	src->add_nativefn( "cprintln", col_println, {}, {}, true );
	return true;
}
