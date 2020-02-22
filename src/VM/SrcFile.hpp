/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef VM_SRC_FILE_HPP
#define VM_SRC_FILE_HPP

#include <vector>
#include <string>

#include "../Common/Errors.hpp"
#include "OpCodes.hpp"
#include "Vars.hpp"

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
	size_t m_id;
	// path and dir are never modified once set
	std::string m_dir;
	std::string m_path;
	std::string m_data; // content
	std::vector< src_col_range_t > m_cols;

	bcode_t m_bcode;

	// used by variable construction mechanism for creating variable of this type
	std::unordered_map< size_t, vartype_base_t * > m_vartypes;
	var_srcfile_t m_vars;

	bool m_is_main;
public:
	/**
	 * \brief Construct a new srcfile_t object
	 * 
	 * \param id id of the instance of srcfile_t
	 * \param dir Directory of source file
	 * \param path Full path of source file
	 * \param is_main Boolean - determins if this source is the main source file (default: false)
	 */
	srcfile_t( const size_t id, const std::string & dir, const std::string & path, const bool is_main = false );

	/**
	 * \brief Loads the file at m_path
	 *
	 * \return Errors E_OK on success, anything else on failure
	 */
	Errors load_file();

	bcode_t & bcode();

	inline void register_vartype( vartype_base_t * type ) { m_vartypes[ type->type() ] = type; }
	vartype_base_t * get_vartype( const size_t & type_id );
	var_srcfile_t & vars();

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
	 * \brief Get the data object
	 * 
	 * \return const std::string& Returns the data object with const qualifier
	 */
	const std::string & get_data() const;

	/**
	 * \brief Return the id of the instance
	 *
	 * \return size_t id of instance
	 */
	size_t get_id() const;

	const std::string & get_path() const;

	/**
	 * \brief Check if the source is main src
	 *
	 * \return bool is main source 
	 */
	bool is_main() const;

	/**
	* \brief Print a proper message on code failure
	*
	* \param idx Index in the code where error is
	* \param msg Error message to show
	* \param ... Format args for error message
	*/
	void fail( const size_t idx, const char * msg, ... ) const;

	/**
	* \brief Print a proper message on code failure
	*
	* \param idx Index in the code where error is
	* \param msg Error message to show
	* \param vargs Format args for error message (va_list style)
	*/
	void fail( const size_t idx, const char * msg, va_list vargs ) const;
};

#endif // VM_SRC_FILE_HPP
