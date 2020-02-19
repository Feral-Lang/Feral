/*
  Copyright (c) 2020, Electrux
  All rights reserved.
  Using the BSD 3-Clause license for the project,
  main LICENSE file resides in project's root directory.
  Please read that file and understand the license terms
  before using or altering the project.
*/

#ifndef VM_VARS_HPP
#define VM_VARS_HPP

#include <string>
#include <unordered_map>

#include "Vars/Base.hpp"

/* vars for each source file */
class vars_t
{
	std::unordered_map< std::string, var_base_t * > m_vars;
};

class var_src_t
{
	std::vector< size_t > m_loops_from;
	vars_t m_src_vars;
	// maps function id to vars_t
	std::unordered_map< size_t, vars_t > m_fn_vars;

};

#endif // VM_VARS_HPP
