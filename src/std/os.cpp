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

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <limits.h>
#include <sys/errno.h> // errno
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "VM/VM.hpp"

int exec_internal(const std::string &file);
std::string dir_part(const std::string &full_loc);

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

var_base_t *sleep_custom(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_int_t>()) {
		vm.fail(fd.src_id, fd.idx, "expected integer argument for sleep time, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(mpz_get_ui(INT(fd.args[1])->get())));
	return vm.nil;
}

var_base_t *get_env(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_str_t>()) {
		vm.fail(fd.src_id, fd.idx,
			"expected string argument for env variable name, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}
	std::string var = STR(fd.args[1])->get();
	const char *env = getenv(var.c_str());
	return make<var_str_t>(env == NULL ? "" : env);
}

var_base_t *set_env(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_str_t>()) {
		vm.fail(fd.src_id, fd.idx,
			"expected string argument for env variable name, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}
	if(!fd.args[2]->istype<var_str_t>()) {
		vm.fail(fd.src_id, fd.idx,
			"expected string argument for env variable value, found: %s",
			vm.type_name(fd.args[2]).c_str());
		return nullptr;
	}
	if(!fd.args[3]->istype<var_bool_t>()) {
		vm.fail(fd.src_id, fd.idx,
			"expected boolean argument for overwrite existing env variable, found: %s",
			vm.type_name(fd.args[3]).c_str());
		return nullptr;
	}
	std::string var = STR(fd.args[1])->get();
	std::string val = STR(fd.args[2])->get();

	bool overwrite = BOOL(fd.args[3])->get();
	return make<var_int_t>(setenv(var.c_str(), val.c_str(), overwrite));
}

var_base_t *exec_custom(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_str_t>()) {
		vm.fail(fd.src_id, fd.idx, "expected string argument for command, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}
	var_vec_t *out = nullptr;
	if(fd.args[2]->istype<var_vec_t>()) {
		out = VEC(fd.args[2]);
	}

	std::string cmd = STR(fd.args[1])->get();

	FILE *pipe = popen(cmd.c_str(), "r");
	if(!pipe) return make<var_int_t>(1);
	char *csline = NULL;
	size_t len   = 0;
	ssize_t nread;

	if(!out) {
		while((nread = getline(&csline, &len, pipe)) != -1) {
			fprintf(stdout, "%s", csline);
		}
	} else {
		std::vector<var_base_t *> &resvec = out->get();
		std::string line;
		while((nread = getline(&csline, &len, pipe)) != -1) {
			line = csline;
			while(line.back() == '\n' || line.back() == '\r') line.pop_back();
			resvec.push_back(new var_str_t(line, fd.src_id, fd.idx));
		}
	}
	if(csline) free(csline);
	int res = pclose(pipe);

	res = WEXITSTATUS(res);
	return make<var_int_t>(res);
}

var_base_t *system_custom(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_str_t>()) {
		vm.fail(fd.src_id, fd.idx, "expected string argument for command, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}

	std::string cmd = STR(fd.args[1])->get();
	int res		= std::system(cmd.c_str());
	res		= WEXITSTATUS(res);

	return make<var_int_t>(res);
}

var_base_t *install(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_str_t>()) {
		vm.fail(fd.src_id, fd.idx, "expected string argument for source, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}
	if(!fd.args[2]->istype<var_str_t>()) {
		vm.fail(fd.src_id, fd.idx, "expected string argument for destination, found: %s",
			vm.type_name(fd.args[2]).c_str());
		return nullptr;
	}
	std::string src = STR(fd.args[1])->get(), dest = STR(fd.args[2])->get();

	if(src.empty() || dest.empty()) {
		return make<var_int_t>(0);
	}

	if(exec_internal("mkdir -p " + dest) != 0) {
		return make<var_int_t>(-1);
	}
	std::string cmd_str;
#if __linux__ || __ANDROID__
	cmd_str = "cp -r --remove-destination " + src + " " + dest;
#elif __APPLE__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__ || __bsdi__ || __DragonFly__
	cmd_str = "cp -rf " + src + " " + dest;
#endif
	return make<var_int_t>(exec_internal(cmd_str));
}

var_base_t *os_get_name(vm_state_t &vm, const fn_data_t &fd)
{
	std::string os_str;
#if __ANDROID__
	os_str = "android";
#elif __linux__
	os_str	= "linux";
#elif __APPLE__
	os_str = "macos";
#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__ || __bsdi__ || __DragonFly__
	os_str = "bsd";
#endif
	return make<var_str_t>(os_str);
}

var_base_t *os_strerr(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_int_t>()) {
		vm.fail(fd.src_id, fd.idx,
			"expected integer argument for destination directory, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}
	return make<var_str_t>(strerror(mpz_get_si(INT(fd.args[1])->get())));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// Extra Functions /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

var_base_t *os_get_cwd(vm_state_t &vm, const fn_data_t &fd)
{
	char cwd[PATH_MAX];
	if(getcwd(cwd, PATH_MAX) == NULL) {
		vm.fail(fd.src_id, fd.idx, "getcwd() failed - internal error");
		return nullptr;
	}
	return make<var_str_t>(cwd);
}

var_base_t *os_set_cwd(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_str_t>()) {
		vm.fail(fd.src_id, fd.idx,
			"expected string argument for destination directory, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}
	return make<var_int_t>(chdir(STR(fd.args[1])->get().c_str()));
}

var_base_t *os_mkdir(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_str_t>()) {
		vm.fail(fd.src_id, fd.idx,
			"expected string argument for destination directory, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}
	std::string dest = STR(fd.args[1])->get();

	for(size_t i = 2; i < fd.args.size(); ++i) {
		if(!fd.args[i]->istype<var_str_t>()) {
			vm.fail(fd.src_id, fd.idx,
				"expected string argument for destination directory, found: %s",
				vm.type_name(fd.args[i]).c_str());
			return nullptr;
		}
		std::string tmpdest = STR(fd.args[i])->get();
		if(tmpdest.empty()) continue;
		dest += " " + tmpdest;
	}
	return make<var_int_t>(exec_internal("mkdir -p " + dest));
}

var_base_t *os_rm(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_str_t>()) {
		vm.fail(fd.src_id, fd.idx,
			"expected string argument for destination directory, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}
	std::string dest = STR(fd.args[1])->get();

	for(size_t i = 2; i < fd.args.size(); ++i) {
		if(!fd.args[i]->istype<var_str_t>()) {
			vm.fail(fd.src_id, fd.idx,
				"expected string argument for destination directory, found: %s",
				vm.type_name(fd.args[i]).c_str());
			return nullptr;
		}
		std::string tmpdest = STR(fd.args[i])->get();
		if(tmpdest.empty()) continue;
		dest += " " + tmpdest;
	}
	if(dest.empty()) {
		return make<var_int_t>(0);
	}
	return make<var_int_t>(exec_internal("rm -r " + dest));
}

var_base_t *os_copy(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_str_t>()) {
		vm.fail(fd.src_id, fd.idx, "expected string argument for source, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}

	std::string src = STR(fd.args[1])->get();
	// last element is the destination
	for(size_t i = 2; i < fd.args.size() - 1; ++i) {
		if(!fd.args[i]->istype<var_str_t>()) {
			vm.fail(fd.src_id, fd.idx,
				"expected string argument for destination directory, found: %s",
				vm.type_name(fd.args[i]).c_str());
			return nullptr;
		}
		std::string tmpdest = STR(fd.args[i])->get();
		if(tmpdest.empty()) continue;
		src += " " + tmpdest;
	}

	if(!fd.args[fd.args.size() - 1]->istype<var_str_t>()) {
		vm.fail(fd.src_id, fd.idx, "expected string argument for source, found: %s",
			vm.type_name(fd.args[fd.args.size() - 1]).c_str());
		return nullptr;
	}

	const std::string &dest = STR(fd.args[fd.args.size() - 1])->get();

	return make<var_int_t>(exec_internal("cp -r " + src + " " + dest));
}

var_base_t *os_chmod(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_str_t>()) {
		vm.fail(fd.src_id, fd.idx, "expected string argument for destination, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}
	if(!fd.args[2]->istype<var_str_t>()) {
		vm.fail(fd.src_id, fd.idx, "expected string argument for mode, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}
	if(!fd.args[3]->istype<var_bool_t>()) {
		vm.fail(fd.src_id, fd.idx, "expected boolean argument for recursive, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}
	const std::string &dest = STR(fd.args[1])->get();
	const std::string &mode = STR(fd.args[2])->get();
	const bool &recurse	= BOOL(fd.args[3])->get();
	std::string cmd		= "chmod ";
	if(recurse) cmd += "-R ";
	cmd += mode + " " + dest;
	return make<var_int_t>(exec_internal(cmd));
}

var_base_t *os_mv(vm_state_t &vm, const fn_data_t &fd)
{
	if(!fd.args[1]->istype<var_str_t>()) {
		vm.fail(fd.src_id, fd.idx, "expected string argument for from, found: %s",
			vm.type_name(fd.args[1]).c_str());
		return nullptr;
	}

	if(!fd.args[2]->istype<var_str_t>()) {
		vm.fail(fd.src_id, fd.idx, "expected string argument for to, found: %s",
			vm.type_name(fd.args[2]).c_str());
		return nullptr;
	}

	const char *from = STR(fd.args[1])->get().c_str();
	const char *to	 = STR(fd.args[2])->get().c_str();

	if(std::rename(from, to) < 0) {
		vm.fail(fd.src_id, fd.idx, "failed to move with error: %s", strerror(errno));
		return nullptr;
	}

	return make<var_int_t>(0);
}

INIT_MODULE(os)
{
	var_src_t *src = vm.current_source();

	src->add_native_fn("sleep", sleep_custom, 1);

	src->add_native_fn("get_env", get_env, 1);
	src->add_native_fn("set_env_native", set_env, 3);

	src->add_native_fn("exec_native", exec_custom, 2);
	src->add_native_fn("system", system_custom, 1);
	src->add_native_fn("install", install, 2);

	src->add_native_fn("os_get_name_native", os_get_name);
	src->add_native_fn("strerr", os_strerr, 1);

	src->add_native_fn("get_cwd", os_get_cwd);
	src->add_native_fn("set_cwd", os_set_cwd, 1);

	src->add_native_fn("mkdir", os_mkdir, 1, true);
	src->add_native_fn("rm", os_rm, 1, true);

	src->add_native_fn("cp", os_copy, 2, true);
	src->add_native_fn("mv", os_mv, 2);

	src->add_native_fn("chmod_native", os_chmod, 3);

	return true;
}

int exec_internal(const std::string &cmd)
{
	FILE *pipe = popen(cmd.c_str(), "r");
	if(!pipe) return 1;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;

	while((nread = getline(&line, &len, pipe)) != -1)
		;
	free(line);
	int res = pclose(pipe);
	return WEXITSTATUS(res);
}

std::string dir_part(const std::string &full_loc)
{
	auto loc = full_loc.find_last_of('/');
	if(loc == std::string::npos) return ".";
	if(loc == 0) return "/";
	return full_loc.substr(0, loc);
}
