#include <iostream>

#include "VM/Interpreter.hpp"

#if defined(FER_OS_WINDOWS)
#include <io.h>

#include "FS.hpp" // For STDOUT_FILENO
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace fer
{

static constexpr size_t MAX_SCAN_LINE_LEN = 1024;

StringRef getColor(StringRef code);
int applyColors(String &str);

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *print(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	   const StringMap<AssnArgData> &assn_args)
{
	ssize_t count = 0;
	for(size_t i = 1; i < args.size(); ++i) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{args[i]};
		if(!vm.callVarAndExpect<VarStr>(loc, "str", v, tmp, {})) return nullptr;
		const String &str = as<VarStr>(v)->getVal();
		count += write(STDOUT_FILENO, str.data(), str.size());
		vm.decVarRef(v);
	}
	return vm.makeVar<VarInt>(loc, count);
}

Var *println(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	     const StringMap<AssnArgData> &assn_args)
{
	ssize_t count = 0;
	for(size_t i = 1; i < args.size(); ++i) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{args[i]};
		if(!vm.callVarAndExpect<VarStr>(loc, "str", v, tmp, {})) return nullptr;
		const String &str = as<VarStr>(v)->getVal();
		count += write(STDOUT_FILENO, str.data(), str.size());
		vm.decVarRef(v);
	}
	count += write(STDOUT_FILENO, "\n", 1);
	return vm.makeVar<VarInt>(loc, count);
}

Var *cprint(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	    const StringMap<AssnArgData> &assn_args)
{
	ssize_t count = 0;
	for(size_t i = 1; i < args.size(); ++i) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{args[i]};
		if(!vm.callVarAndExpect<VarStr>(loc, "str", v, tmp, {})) return nullptr;
		String str = as<VarStr>(v)->getVal();
		applyColors(str);
		count += write(STDOUT_FILENO, str.data(), str.size());
		vm.decVarRef(v);
	}
	return vm.makeVar<VarInt>(loc, count);
}

Var *cprintln(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	ssize_t count = 0;
	for(size_t i = 1; i < args.size(); ++i) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{args[i]};
		if(!vm.callVarAndExpect<VarStr>(loc, "str", v, tmp, {})) return nullptr;
		String str = as<VarStr>(v)->getVal();
		applyColors(str);
		count += write(STDOUT_FILENO, str.data(), str.size());
		vm.decVarRef(v);
	}
	count += write(STDOUT_FILENO, "\n", 1);
	return vm.makeVar<VarInt>(loc, count);
}

Var *eprint(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	    const StringMap<AssnArgData> &assn_args)
{
	ssize_t count = 0;
	for(size_t i = 1; i < args.size(); ++i) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{args[i]};
		if(!vm.callVarAndExpect<VarStr>(loc, "str", v, tmp, {})) return nullptr;
		const String &str = as<VarStr>(v)->getVal();
		count += write(STDERR_FILENO, str.data(), str.size());
		vm.decVarRef(v);
	}
	return vm.makeVar<VarInt>(loc, count);
}

Var *eprintln(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	ssize_t count = 0;
	for(size_t i = 1; i < args.size(); ++i) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{args[i]};
		if(!vm.callVarAndExpect<VarStr>(loc, "str", v, tmp, {})) return nullptr;
		const String &str = as<VarStr>(v)->getVal();
		count += write(STDERR_FILENO, str.data(), str.size());
		vm.decVarRef(v);
	}
	count += write(STDERR_FILENO, "\n", 1);
	return vm.makeVar<VarInt>(loc, count);
}

Var *ecprint(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	     const StringMap<AssnArgData> &assn_args)
{
	ssize_t count = 0;
	for(size_t i = 1; i < args.size(); ++i) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{args[i]};
		if(!vm.callVarAndExpect<VarStr>(loc, "str", v, tmp, {})) return nullptr;
		String str = as<VarStr>(v)->getVal();
		applyColors(str);
		count += write(STDERR_FILENO, str.data(), str.size());
		vm.decVarRef(v);
	}
	return vm.makeVar<VarInt>(loc, count);
}

Var *ecprintln(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	       const StringMap<AssnArgData> &assn_args)
{
	ssize_t count = 0;
	for(size_t i = 1; i < args.size(); ++i) {
		Var *v = nullptr;
		Array<Var *, 1> tmp{args[i]};
		if(!vm.callVarAndExpect<VarStr>(loc, "str", v, tmp, {})) return nullptr;
		String str = as<VarStr>(v)->getVal();
		applyColors(str);
		count += write(STDERR_FILENO, str.data(), str.size());
		vm.decVarRef(v);
	}
	count += write(STDERR_FILENO, "\n", 1);
	return vm.makeVar<VarInt>(loc, count);
}

Var *fprint(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	    const StringMap<AssnArgData> &assn_args)
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
		if(!vm.callVarAndExpect<VarStr>(loc, "str", v, tmp, {})) return nullptr;
		const String &str = as<VarStr>(v)->getVal();
		count += fwrite(str.data(), sizeof(char), str.size(), f);
		vm.decVarRef(v);
	}
	return vm.makeVar<VarInt>(loc, count);
}

Var *fprintln(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
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
		if(!vm.callVarAndExpect<VarStr>(loc, "str", v, tmp, {})) return nullptr;
		const String &str = as<VarStr>(v)->getVal();
		count += fwrite(str.data(), sizeof(char), str.size(), f);
		vm.decVarRef(v);
	}
	count += fwrite("\n", sizeof(char), 1, f);
	return vm.makeVar<VarInt>(loc, count);
}

Var *fcprint(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	     const StringMap<AssnArgData> &assn_args)
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
		if(!vm.callVarAndExpect<VarStr>(loc, "str", v, tmp, {})) return nullptr;
		String str = as<VarStr>(v)->getVal();
		applyColors(str);
		count += fwrite(str.data(), sizeof(char), str.size(), f);
		vm.decVarRef(v);
	}
	return vm.makeVar<VarInt>(loc, count);
}

Var *fcprintln(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	       const StringMap<AssnArgData> &assn_args)
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
		if(!vm.callVarAndExpect<VarStr>(loc, "str", v, tmp, {})) return nullptr;
		String str = as<VarStr>(v)->getVal();
		applyColors(str);
		count += fwrite(str.data(), sizeof(char), str.size(), f);
		vm.decVarRef(v);
	}
	count += fwrite("\n", sizeof(char), 1, f);
	return vm.makeVar<VarInt>(loc, count);
}

Var *scan(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	  const StringMap<AssnArgData> &assn_args)
{
	if(args.size() > 1 && !args[1]->is<VarStr>()) {
		vm.fail(args[1]->getLoc(),
			"expected string data for input prompt, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}

	if(args.size() > 1) {
		const String &prompt = as<VarStr>(args[1])->getVal();
		write(STDOUT_FILENO, prompt.data(), prompt.size());
	}

	VarStr *res = vm.makeVar<VarStr>(loc, "");
	std::getline(std::cin, res->getVal());

	if(!res->getVal().empty() && res->getVal().back() == '\r') res->getVal().pop_back();
	if(!res->getVal().empty() && res->getVal().back() == '\n') res->getVal().pop_back();

	return res;
}

Var *scanEOF(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	     const StringMap<AssnArgData> &assn_args)
{
	if(args.size() > 1 && !args[1]->is<VarStr>()) {
		vm.fail(args[1]->getLoc(),
			"expected string data for input prompt, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}

	if(args.size() > 1) {
		const String &prompt = as<VarStr>(args[1])->getVal();
		write(STDOUT_FILENO, prompt.data(), prompt.size());
	}

	String line;
	VarStr *res = vm.makeVar<VarStr>(loc, "");
	while(std::getline(std::cin, line)) res->getVal() += line;

	if(!res->getVal().empty() && res->getVal().back() == '\r') res->getVal().pop_back();
	if(!res->getVal().empty() && res->getVal().back() == '\n') res->getVal().pop_back();

	return res;
}

Var *fflush(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	    const StringMap<AssnArgData> &assn_args)
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

Var *readChar(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(args[1]->getLoc(),
			"expected an integer argument for file descriptor, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}

	int fd	= as<VarInt>(args[1])->getVal();
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

	mod->addNativeFn(vm, "print", print, 1, true);
	mod->addNativeFn(vm, "println", println, 0, true);
	mod->addNativeFn(vm, "cprint", cprint, 1, true);
	mod->addNativeFn(vm, "cprintln", cprintln, 0, true);
	mod->addNativeFn(vm, "eprint", eprint, 1, true);
	mod->addNativeFn(vm, "eprintln", eprintln, 0, true);
	mod->addNativeFn(vm, "ecprint", ecprint, 1, true);
	mod->addNativeFn(vm, "ecprintln", ecprintln, 0, true);
	mod->addNativeFn(vm, "fprint", fprint, 2, true);
	mod->addNativeFn(vm, "fprintln", fprintln, 1, true);
	mod->addNativeFn(vm, "fcprint", fcprint, 1, true);
	mod->addNativeFn(vm, "fcprintln", fcprintln, 0, true);
	mod->addNativeFn(vm, "scan", scan, 0, true);
	mod->addNativeFn(vm, "scanEOF", scanEOF, 0, true);
	mod->addNativeFn(vm, "fflush", fflush, 1);
	mod->addNativeFn(vm, "readChar", readChar, 1);

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

} // namespace fer