#include <cstdarg>

#include "SrcFile.hpp"

srcfile_t::srcfile_t( const size_t id, const std::string & dir, const std::string & path, const bool is_main )
	: m_id( id ), m_dir( dir ), m_path( path ), m_is_main( false ) {}

Errors srcfile_t::load_file()
{
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	fp = fopen( m_path.c_str(), "r" );
	if( fp == NULL ) {
		fprintf( stderr, "failed to open source file: %s\n", m_path.c_str() );
		return E_FILE_IO;
	}

	size_t prefix_idx = get_data().size();
	std::string code;
	std::vector< src_col_range_t > cols;
	size_t begin, end;
	while( ( read = getline( & line, & len, fp ) ) != -1 ) {
		begin = code.size();
		code += line;
		end = code.size();
		cols.push_back( { prefix_idx + begin, prefix_idx + end } );
	}

	fclose( fp );
	if( line ) free( line );

	if( code.empty() ) {
		fprintf( stderr, "encountered empty file: %s\n", m_path.c_str() );
		return E_FILE_EMPTY;
	}

	add_data( code );
	add_cols( cols );
	return E_OK;
}

void srcfile_t::add_data( const std::string & data ) { m_data += data; }
void srcfile_t::add_cols( const std::vector< src_col_range_t > & cols )
{ m_cols.insert( m_cols.end(), cols.begin(), cols.end() ); }

const std::string & srcfile_t::get_data() const { return m_data; }
size_t srcfile_t::get_id() const { return m_id; }
bool srcfile_t::is_main() const { return m_is_main; }

void srcfile_t::fail( const size_t idx, const char * msg, ... ) const
{
	size_t line, col_begin, col_end, col;
	bool found = false;
	for( size_t i = 0; i < m_cols.size(); ++i ) {
		if( idx >= m_cols[ i ].begin && idx < m_cols[ i ].end ) {
			line = i;
			col_begin = m_cols[ i ].begin;
			col_end = m_cols[ i ].end;
			col = idx - col_begin;
			found = true;
			break;
		}
	}
	if( !found ) {
		fprintf( stderr, "could not find error: " );
		va_list vargs;
		va_start( vargs, msg );
		vfprintf( stderr, msg, vargs );
		fprintf( stderr, "\n" );
		va_end( vargs );
		fprintf( stderr, "in file: %s, with index: %zu\n", m_path.c_str(), idx );
		return;
	}

	fprintf( stderr, "%s %zu[%zu]: error: ", m_path.c_str(), line, col );

	va_list vargs;
	va_start( vargs, msg );
	vfprintf( stderr, msg, vargs );
	fprintf( stderr, "\n" );
	va_end( vargs );

	std::string err_line = m_data.substr( col_begin, col_end - col_begin );
	fprintf( stderr, "%s", err_line.c_str() );

	std::string spcs;
	int tab_count = 0;
	for( auto & ch : err_line ) {
		if( ch == '\t' ) ++tab_count;
	}
	for( int i = 0; i < tab_count; ++i ) spcs += '\t';
	for( int i = 0; i < col - tab_count; ++i ) {
		spcs += " ";
	}
	fprintf( stderr, "%s^\n", spcs.c_str() );
}
