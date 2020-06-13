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

#ifndef VM_SRC_FILE_HPP
#define VM_SRC_FILE_HPP

#include <vector>
#include <string>

#include "../Common/Errors.hpp"
#include "OpCodes.hpp"

/**
 * \brief Defines the range of columns for a line of source code
 *
 */
struct src_col_range_t
{
	size_t begin;
	size_t end;
};

/**
 * \brief Defines a source code file object, contains its name, directory,
 * content, and line divisions (based on indices)
 *
 */
class srcfile_t
{
	// set automatically for each source (guaranteed to be unique)
	size_t m_id;
	// path and dir are never modified once set
	std::string m_dir;
	std::string m_path;
	std::string m_data; // content
	std::vector< src_col_range_t > m_cols;

	bcode_t m_bcode;

	bool m_is_main;
public:
	/**
	 * \brief Construct a new srcfile_t object
	 *
	 * \param dir Directory of source file
	 * \param path Full path of source file
	 * \param is_main Boolean - determins if this source is the main source file (default: false)
	 */
	srcfile_t( const std::string & dir, const std::string & path, const bool is_main = false );

	/**
	 * \brief Construct a new srcfile_t object (do not call load_file() after this)
	 *
	 * \param dir Directory of source file
	 * \param path Full path of source file
	 * \param path Full path of source file
	 * \param data this will be the data instead of reading from file
	 * \param is_main Boolean - determins if this source is the main source file (default: false)
	 */
	srcfile_t( const std::string & dir, const std::string & path, const std::string & data, const bool is_main = false );

	/**
	 * \brief Loads the file at m_path
	 *
	 * \return Errors E_OK on success, anything else on failure
	 */
	Errors load_file();

	/**
	 * \brief Append content to an instance
	 *
	 * \param data The content to be appended
	 */
	void add_data( const std::string & data );
	/**
	 * \brief Append line divisions to an instance
	 *
	 * \param cols Line divisions to be appended (vector of src_col_range_t)
	 */
	void add_cols( const std::vector< src_col_range_t > & cols );

	/**
	 * \brief Return the id of the instance
	 *
	 * \return size_t id of instance
	 */
	inline size_t id() const { return m_id; }

	/**
	 * \brief Return the directory part of the source file
	 *
	 * \return string directory part of source file
	 */
	inline const std::string & dir() const { return m_dir; }

	/**
	 * \brief Return the full path of the source file
	 *
	 * \return string path of source file
	 */
	inline const std::string & path() const { return m_path; }

	/**
	 * \brief Get the data object
	 *
	 * \return const std::string& Returns the data object with const qualifier
	 */
	inline const std::string & data() const { return m_data; }

	/**
	 * \brief Get the bcode_t
	 *
	 * \return bcode_t& Returns the bcode_t object
	 */
	bcode_t & bcode() { return m_bcode; }

	/**
	 * \brief Check if the source is main src
	 *
	 * \return bool is main source
	 */
	inline bool is_main() const { return m_is_main; }

	/**
	* \brief Print a proper message on code failure
	*
	* \param idx Index in the code where error is
	* \param msg Error message to show
	* \param ... Format args for error message
	*/
	void fail( const size_t & idx, const char * msg, ... ) const;

	/**
	* \brief Print a proper message on code failure
	*
	* \param idx Index in the code where error is
	* \param msg Error message to show
	* \param vargs Format args for error message (va_list style)
	*/
	void fail( const size_t & idx, const char * msg, va_list vargs ) const;
};

#endif // VM_SRC_FILE_HPP
