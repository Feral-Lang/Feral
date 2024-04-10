#include <iostream>

#include "std/FSType.hpp"
#include "VM/Interpreter.hpp"

#if defined(FER_OS_WINDOWS)
#include <io.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

static constexpr size_t MAX_SCAN_LINE_LEN = 1024;

StringRef getColor(StringRef code);
int applyColors(String &str);

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *print(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	   const Map<String, AssnArgData> &assn_args)
{
	ssize_t count = 0;
	for(size_t i = 1; i < args.size(); ++i) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{args[i]};
		if(!vm.callFn(loc, "str", v, tmp, {})) return nullptr;
		if(!v->is<VarStr>()) {
			vm.fail(loc,
				"'str' member call did not return a"
				" string, instead returned: ",
				vm.getTypeName(v));
			decref(v);
			return nullptr;
		}
		const String &str = as<VarStr>(v)->get();
		count += write(STDOUT_FILENO, str.data(), str.size());
		decref(v);
	}
	return vm.makeVar<VarInt>(loc, count);
}

Var *println(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const Map<String, AssnArgData> &assn_args)
{
	ssize_t count = 0;
	for(size_t i = 1; i < args.size(); ++i) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{args[i]};
		if(!vm.callFn(loc, "str", v, tmp, {})) return nullptr;
		if(!v->is<VarStr>()) {
			vm.fail(loc,
				"'str' member call did not return a"
				" string, instead returned: ",
				vm.getTypeName(v));
			decref(v);
			return nullptr;
		}
		const String &str = as<VarStr>(v)->get();
		count += write(STDOUT_FILENO, str.data(), str.size());
		decref(v);
	}
	count += write(STDOUT_FILENO, "\n", 1);
	return vm.makeVar<VarInt>(loc, count);
}

Var *fprint(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarFile>()) {
		vm.fail(args[1]->getLoc(),
			"expected a file argument for fprint, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	if(as<VarFile>(args[1])->getFile() == nullptr) {
		vm.fail(args[1]->getLoc(), "file has probably been closed already",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	FILE *f	      = as<VarFile>(args[1])->getFile();
	ssize_t count = 0;
	for(size_t i = 2; i < args.size(); ++i) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{args[i]};
		if(!vm.callFn(loc, "str", v, tmp, {})) return nullptr;
		if(!v->is<VarStr>()) {
			vm.fail(loc,
				"'str' member call did not return a"
				" string, instead returned: ",
				vm.getTypeName(v));
			decref(v);
			return nullptr;
		}
		const String &str = as<VarStr>(v)->get();
		count += fwrite(str.data(), sizeof(char), str.size(), f);
		decref(v);
	}
	return vm.makeVar<VarInt>(loc, count);
}

Var *fprintln(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarFile>()) {
		vm.fail(args[1]->getLoc(),
			"expected a file argument for fprint, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	if(as<VarFile>(args[1])->getFile() == nullptr) {
		vm.fail(args[1]->getLoc(), "file has probably been closed already",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	FILE *f	      = as<VarFile>(args[1])->getFile();
	ssize_t count = 0;
	for(size_t i = 2; i < args.size(); ++i) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{args[i]};
		if(!vm.callFn(loc, "str", v, tmp, {})) return nullptr;
		if(!v->is<VarStr>()) {
			vm.fail(loc,
				"'str' member call did not return a"
				" string, instead returned: ",
				vm.getTypeName(v));
			decref(v);
			return nullptr;
		}
		const String &str = as<VarStr>(v)->get();
		count += fwrite(str.data(), sizeof(char), str.size(), f);
		decref(v);
	}
	count += fwrite("\n", sizeof(char), 1, f);
	return vm.makeVar<VarInt>(loc, count);
}

Var *cprint(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const Map<String, AssnArgData> &assn_args)
{
	ssize_t count = 0;
	for(size_t i = 1; i < args.size(); ++i) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{args[i]};
		if(!vm.callFn(loc, "str", v, tmp, {})) return nullptr;
		if(!v->is<VarStr>()) {
			vm.fail(loc,
				"'str' member call did not return a"
				" string, instead returned: ",
				vm.getTypeName(v));
			decref(v);
			return nullptr;
		}
		String str = as<VarStr>(v)->get();
		applyColors(str);
		count += write(STDOUT_FILENO, str.data(), str.size());
		decref(v);
	}
	return vm.makeVar<VarInt>(loc, count);
}

Var *cprintln(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	ssize_t count = 0;
	for(size_t i = 1; i < args.size(); ++i) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{args[i]};
		if(!vm.callFn(loc, "str", v, tmp, {})) return nullptr;
		if(!v->is<VarStr>()) {
			vm.fail(loc,
				"'str' member call did not return a"
				" string, instead returned: ",
				vm.getTypeName(v));
			decref(v);
			return nullptr;
		}
		String str = as<VarStr>(v)->get();
		applyColors(str);
		count += write(STDOUT_FILENO, str.data(), str.size());
		decref(v);
	}
	count += write(STDOUT_FILENO, "\n", 1);
	return vm.makeVar<VarInt>(loc, count);
}

Var *fcprint(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarFile>()) {
		vm.fail(args[1]->getLoc(),
			"expected a file argument for fcprint, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	if(as<VarFile>(args[1])->getFile() == nullptr) {
		vm.fail(args[1]->getLoc(), "file has probably been closed already",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	FILE *f	      = as<VarFile>(args[1])->getFile();
	ssize_t count = 0;
	for(size_t i = 2; i < args.size(); ++i) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{args[i]};
		if(!vm.callFn(loc, "str", v, tmp, {})) return nullptr;
		if(!v->is<VarStr>()) {
			vm.fail(loc,
				"'str' member call did not return a"
				" string, instead returned: ",
				vm.getTypeName(v));
			decref(v);
			return nullptr;
		}
		String str = as<VarStr>(v)->get();
		applyColors(str);
		count += fwrite(str.data(), sizeof(char), str.size(), f);
		decref(v);
	}
	return vm.makeVar<VarInt>(loc, count);
}

Var *fcprintln(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarFile>()) {
		vm.fail(args[1]->getLoc(),
			"expected a file argument for fcprint, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	if(as<VarFile>(args[1])->getFile() == nullptr) {
		vm.fail(args[1]->getLoc(), "file has probably been closed already",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	FILE *f	      = as<VarFile>(args[1])->getFile();
	ssize_t count = 0;
	for(size_t i = 2; i < args.size(); ++i) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{args[i]};
		if(!vm.callFn(loc, "str", v, tmp, {})) return nullptr;
		if(!v->is<VarStr>()) {
			vm.fail(loc,
				"'str' member call did not return a"
				" string, instead returned: ",
				vm.getTypeName(v));
			decref(v);
			return nullptr;
		}
		String str = as<VarStr>(v)->get();
		applyColors(str);
		count += fwrite(str.data(), sizeof(char), str.size(), f);
		decref(v);
	}
	count += write(STDOUT_FILENO, "\n", 1);
	return vm.makeVar<VarInt>(loc, count);
}

Var *scan(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	  const Map<String, AssnArgData> &assn_args)
{
	if(args.size() > 1 && !args[1]->is<VarStr>()) {
		vm.fail(args[1]->getLoc(),
			"expected string data for input prompt, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}

	if(args.size() > 1) {
		const String &prompt = as<VarStr>(args[1])->get();
		write(STDOUT_FILENO, prompt.data(), prompt.size());
	}

	VarStr *res = vm.makeVar<VarStr>(loc, "");
	std::getline(std::cin, res->get());

	if(!res->get().empty() && res->get().back() == '\r') res->get().pop_back();
	if(!res->get().empty() && res->get().back() == '\n') res->get().pop_back();

	return res;
}

Var *scanEOF(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const Map<String, AssnArgData> &assn_args)
{
	if(args.size() > 1 && !args[1]->is<VarStr>()) {
		vm.fail(args[1]->getLoc(),
			"expected string data for input prompt, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}

	if(args.size() > 1) {
		const String &prompt = as<VarStr>(args[1])->get();
		write(STDOUT_FILENO, prompt.data(), prompt.size());
	}

	String line;
	VarStr *res = vm.makeVar<VarStr>(loc, "");
	while(std::getline(std::cin, line)) res->get() += line;

	if(!res->get().empty() && res->get().back() == '\r') res->get().pop_back();
	if(!res->get().empty() && res->get().back() == '\n') res->get().pop_back();

	return res;
}

Var *fflush(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarFile>()) {
		vm.fail(args[1]->getLoc(),
			"expected a file argument for fcprint, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	if(as<VarFile>(args[1])->getFile() == nullptr) {
		vm.fail(args[1]->getLoc(), "file has probably been closed already",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	fflush(as<VarFile>(args[1])->getFile());
	return vm.getNil();
}

Var *readChar(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(args[1]->getLoc(),
			"expected an integer argument for file descriptor, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}

	int fd	= as<VarInt>(args[1])->get();
	char c	= 0;
	int res = read(fd, &c, 1);
	if(res <= 0) {
		vm.fail(loc, "failed to read char");
		return nullptr;
	}
	return vm.makeVar<VarStr>(loc, c);
}

INIT_MODULE(IO)
{
	VarModule *mod = vm.getCurrModule();

	mod->addNativeFn("print", print, 1, true);
	mod->addNativeFn("println", println, 0, true);
	mod->addNativeFn("fprint", fprint, 2, true);
	mod->addNativeFn("fprintln", fprintln, 1, true);
	mod->addNativeFn("cprint", cprint, 1, true);
	mod->addNativeFn("cprintln", cprintln, 0, true);
	mod->addNativeFn("fcprint", fcprint, 1, true);
	mod->addNativeFn("fcprintln", fcprintln, 0, true);
	mod->addNativeFn("scan", scan, 0, true);
	mod->addNativeFn("scanEOF", scanEOF, 0, true);
	mod->addNativeFn("fflush", fflush, 1);
	mod->addNativeFn("readChar", readChar, 1);

	// stdin, stdout, and stderr cannot be owned by a VarFile
	mod->addNativeVar("stdin", vm.makeVar<VarFile>(loc, stdin, "r", false));
	mod->addNativeVar("stdout", vm.makeVar<VarFile>(loc, stdout, "w", false));
	mod->addNativeVar("stderr", vm.makeVar<VarFile>(loc, stderr, "w", false));
	return true;
}

StringRef getColor(StringRef code)
{
	// reset
	if(code == "0") return "\033[0m";

	// regular
	else if(code == "r") return "\033[0;31m";
	else if(code == "g") return "\033[0;32m";
	else if(code == "y") return "\033[0;33m";
	else if(code == "b") return "\033[0;34m";
	else if(code == "m") return "\033[0;35m";
	else if(code == "c") return "\033[0;36m";
	else if(code == "w") return "\033[0;37m";

	// bold
	else if(code == "br") return "\033[1;31m";
	else if(code == "bg") return "\033[1;32m";
	else if(code == "by") return "\033[1;33m";
	else if(code == "bb") return "\033[1;34m";
	else if(code == "bm") return "\033[1;35m";
	else if(code == "bc") return "\033[1;36m";
	else if(code == "bw") return "\033[1;37m";

	return "";
}

int applyColors(String &str)
{
	int chars = 0;
	for(size_t i = 0; i < str.size();) {
		if(str[i] == '{' && (i == 0 || (str[i - 1] != '$' && str[i - 1] != '%' &&
						str[i - 1] != '#' && str[i - 1] != '\\')))
		{
			str.erase(str.begin() + i);
			if(i < str.size() && str[i] == '{') {
				++i;
				++chars;
				continue;
			}

			String var;
			while(i < str.size() && str[i] != '}') {
				var += str[i];
				str.erase(str.begin() + i);
			}
			// Remove the ending brace
			if(i < str.size()) str.erase(str.begin() + i);

			if(var.empty()) continue;

			StringRef col = getColor(var);
			if(!col.empty()) {
				str.insert(str.begin() + i, col.begin(), col.end());
				i += col.size();
			}
		} else {
			++i;
			++chars;
		}
	}
	return chars;
}