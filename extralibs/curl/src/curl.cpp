/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <curl/curl.h>

#include <feral/VM/VM.hpp>

static std::string progress_style = "percent";
static char progress_bar_filled_char = '=';
static char progress_bar_left_char = ' ';
// how wide the progress bar should be
static int progress_bar_length = 50;

int progress_func( void * ptr, curl_off_t to_download, curl_off_t downloaded, 
		   curl_off_t to_upload, curl_off_t uploaded )
{
	// ensure that the file to be downloaded is not empty
	// because that would cause a division by zero error later on
	if( to_download <= 0 ) {
		return 0;
	}

	double fraction_downloaded = ( double )downloaded / to_download;

	// part of the progressmeter that's already "full"
	int filled_dots = round( fraction_downloaded * progress_bar_length );

	// create the "meter"
	int i = 0;
	if( progress_style == "percent" ) {
		fprintf( stdout, "%3.0f%%\r", fraction_downloaded * 100 );
	} else if( progress_style == "bar" ) {
		fprintf( stdout, "%3.0f%% [", fraction_downloaded * 100 );
		// part that's full already
		for ( ; i < filled_dots; ++i ) {
			fprintf( stdout, "%c", progress_bar_filled_char );
		}
		// remaining part (spaces)
		for ( ; i < progress_bar_length; ++i ) {
			fprintf( stdout, "%c", progress_bar_left_char );
		}
		// and back to line begin
		fprintf( stdout, "]\r" );
	}
	fflush( stdout );
	// if you don't return 0, the transfer will be aborted - see the documentation
	return 0; 
}

var_base_t * curl_download( vm_state_t & vm, const fn_data_t & fd )
{
	srcfile_t * src_file = vm.src_stack.back()->src();

	if( fd.args[ 1 ]->type() != VT_STR ) {
		src_file->fail( fd.args[ 1 ]->idx(), "expected URL to be of type 'str', found: %s",
				vm.type_name( fd.args[ 1 ]->type() ).c_str() );
		return nullptr;
	}

	if( fd.args[ 2 ]->type() != VT_STR ) {
		src_file->fail( fd.args[ 2 ]->idx(), "expected output file name to be of type 'str', found: %s",
				vm.type_name( fd.args[ 2 ]->type() ).c_str() );
		return nullptr;
	}

	const std::string & url = STR( fd.args[ 1 ] )->get();
	const std::string & out = STR( fd.args[ 2 ] )->get();

	if( fd.assn_args_loc.find( "progress" ) != fd.assn_args_loc.end() ) {
		auto & progress_arg = fd.assn_args[ fd.assn_args_loc.at( "progress" ) ];
		var_base_t * progress_val = progress_arg.val;
		if( progress_val->type() != VT_STR ) {
			src_file->fail( progress_arg.idx, "expected 'progress' argument to be of type 'str', found: %s",
					vm.type_name( progress_val->type() ).c_str() );
			return nullptr;
		}
		progress_style = STR( progress_val )->get();
	}

	if( progress_style == "bar" ) {
		if( fd.assn_args_loc.find( "bar_filled" ) != fd.assn_args_loc.end() ) {
			auto & bar_filled_style = fd.assn_args[ fd.assn_args_loc.at( "bar_filled" ) ];
			var_base_t * bar_filled_style_val = bar_filled_style.val;
			if( bar_filled_style_val->type() != VT_STR ) {
				src_file->fail( bar_filled_style.idx, "expected 'bar_filled' argument to be of type 'str', found: %s",
						vm.type_name( bar_filled_style_val->type() ).c_str() );
				return nullptr;
			}
			std::string & bar_filled_style_str = STR( bar_filled_style_val )->get();
			progress_bar_filled_char = bar_filled_style_str.size() > 0 ? bar_filled_style_str[ 0 ] : '=';
		}
		if( fd.assn_args_loc.find( "bar_left" ) != fd.assn_args_loc.end() ) {
			auto & bar_left_style = fd.assn_args[ fd.assn_args_loc.at( "bar_left" ) ];
			var_base_t * bar_left_style_val = bar_left_style.val;
			if( bar_left_style_val->type() != VT_STR ) {
				src_file->fail( bar_left_style.idx, "expected 'bar_left' argument to be of type 'str', found: %s",
						vm.type_name( bar_left_style_val->type() ).c_str() );
				return nullptr;
			}
			std::string & bar_left_style_str = STR( bar_left_style_val )->get();
			progress_bar_left_char = bar_left_style_str.size() > 0 ? bar_left_style_str[ 0 ] : '+';
		}
		if( fd.assn_args_loc.find( "bar_len" ) != fd.assn_args_loc.end() ) {
			auto & bar_len_arg = fd.assn_args[ fd.assn_args_loc.at( "bar_len" ) ];
			var_base_t * bar_len_val = bar_len_arg.val;
			if( bar_len_val->type() != VT_INT ) {
				src_file->fail( bar_len_arg.idx, "expected 'bar_len' argument to be of type 'int', found: %s",
						vm.type_name( bar_len_val->type() ).c_str() );
				return nullptr;
			}
			progress_bar_length = INT( bar_len_val )->get().get_ui();
		}
	}

	CURL * curl;
	FILE * fp;
	CURLcode res;
	curl = curl_easy_init();
	if( !curl ) {
		src_file->fail( fd.idx, "curl_easy_init() failed" );
		return nullptr;
	}

	fp = fopen( out.c_str(), "wb" );
	curl_easy_setopt( curl, CURLOPT_URL, url.c_str() );
	curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );
	curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, NULL );
	curl_easy_setopt( curl, CURLOPT_NOPROGRESS, 0L );
	curl_easy_setopt( curl, CURLOPT_XFERINFOFUNCTION, progress_func );
	curl_easy_setopt( curl, CURLOPT_XFERINFODATA, NULL );
	curl_easy_setopt( curl, CURLOPT_WRITEDATA, fp );
	res = curl_easy_perform( curl );
	curl_easy_cleanup( curl);
	fclose( fp );

	return make< var_int_t >( res );
}

REGISTER_MODULE( curl )
{
	var_src_t * src = vm.src_stack.back();
	const std::string & src_name = src->src()->path();
	src->add_nativefn( "download", curl_download, { "", "" } );
	return true;
}
