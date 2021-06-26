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

#include "VM/Vars/Base.hpp"

#include "VM/Memory.hpp"
#include "VM/VM.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// VAR_BASE /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

var_base_t::var_base_t(const std::uintptr_t &type, const size_t &src_id, const size_t &idx,
		       const bool &callable, const bool &attr_based)
	: m_type(type), m_src_id(src_id), m_idx(idx), m_ref(1), m_info(0)
{
	if(callable) m_info |= VI_CALLABLE;
	if(attr_based) m_info |= VI_ATTR_BASED;
}
var_base_t::~var_base_t() {}

std::uintptr_t var_base_t::typefn_id() const
{
	return m_type;
}

bool var_base_t::to_str(vm_state_t &vm, std::string &data, const size_t &src_id, const size_t &idx)
{
	var_base_t *str_fn = nullptr;
	if(attr_based()) str_fn = attr_get("str");
	if(str_fn == nullptr) str_fn = vm.get_typefn(this, "str");

	if(!str_fn) {
		vm.fail(this->src_id(), this->idx(),
			"no 'str' function implemented for type: '%zu' or global type",
			this->type());
		return false;
	}
	if(!str_fn->call(vm, {this}, {}, {}, src_id, idx)) {
		vm.fail(this->src_id(), this->idx(), "function call 'str' for type: %zu failed",
			this->type());
		return false;
	}
	var_base_t *str = vm.vm_stack->pop(false);
	if(!str->istype<var_str_t>()) {
		vm.fail(this->src_id(), this->idx(),
			"expected string return type from 'str' function, received: %s",
			vm.type_name(str).c_str());
		var_dref(str);
		return false;
	}
	data = STR(str)->get();
	var_dref(str);
	return true;
}

bool var_base_t::to_bool(vm_state_t &vm, bool &data, const size_t &src_id, const size_t &idx)
{
	var_base_t *bool_fn = nullptr;
	if(attr_based()) bool_fn = attr_get("bool");
	if(bool_fn == nullptr) bool_fn = vm.get_typefn(this, "bool");

	if(!bool_fn) {
		vm.fail(this->src_id(), this->idx(),
			"no 'bool' function implemented for type: '%zu' or global type",
			this->type());
		return false;
	}
	if(!bool_fn->call(vm, {this}, {}, {}, src_id, idx)) {
		vm.fail(this->src_id(), this->idx(), "function call 'bool' for type: %zu failed",
			this->type());
		return false;
	}
	var_base_t *b = vm.vm_stack->pop(false);
	if(!b->istype<var_bool_t>()) {
		vm.fail(this->src_id(), this->idx(),
			"expected string return type from 'bool' function, received: %s",
			vm.type_name(b).c_str());
		var_dref(b);
		return false;
	}
	data = BOOL(b)->get();
	var_dref(b);
	return true;
}

var_base_t *var_base_t::call(vm_state_t &vm, const std::vector<var_base_t *> &args,
			     const std::vector<fn_assn_arg_t> &assn_args,
			     const std::unordered_map<std::string, size_t> &assn_args_loc,
			     const size_t &src_id, const size_t &idx)
{
	return nullptr;
}

bool var_base_t::attr_exists(const std::string &name) const
{
	return false;
}
void var_base_t::attr_set(const std::string &name, var_base_t *val, const bool iref) {}
var_base_t *var_base_t::attr_get(const std::string &name)
{
	return nullptr;
}

void *var_base_t::operator new(size_t sz)
{
	return mem::alloc(sz);
}
void var_base_t::operator delete(void *ptr, size_t sz)
{
	mem::free(ptr, sz);
}

void init_typenames(vm_state_t &vm)
{
	vm.register_type<var_all_t>("all");

	vm.register_type<var_nil_t>("nil");
	vm.register_type<var_typeid_t>("typeid");
	vm.register_type<var_bool_t>("bool");
	vm.register_type<var_int_t>("int");
	vm.register_type<var_flt_t>("flt");
	vm.register_type<var_str_t>("str");
	vm.register_type<var_vec_t>("vec");
	vm.register_type<var_map_t>("map");
	vm.register_type<var_fn_t>("func");
	vm.register_type<var_src_t>("src");
}
