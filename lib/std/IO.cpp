#include <iostream>

#include "VM/Interpreter.hpp"

#if defined(CORE_OS_WINDOWS)
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

inline ssize_t writeToFile(FILE *file, StringRef data)
{
    return fwrite(data.data(), sizeof(char), data.size(), file);
}
inline ssize_t writeToFileWithCol(FILE *file, StringRef data)
{
    String s(data);
    applyColors(s);
    return writeToFile(file, s);
}
ssize_t printBase(VirtualMachine &vm, ModuleLoc loc, FILE *file, Span<Var *> args, bool withCols)
{
    ssize_t count = 0;
    for(auto &a : args) {
        if(a->is<VarStr>()) {
            const String &s = as<VarStr>(a)->getVal();
            count += withCols ? writeToFileWithCol(file, s) : writeToFile(file, s);
            continue;
        }
        Var *v = nullptr;
        Array<Var *, 1> tmp{a};
        if(!vm.callVarAndExpect<VarStr>(loc, "str", v, tmp, {})) return -1;
        const String &s = as<VarStr>(v)->getVal();
        count += withCols ? writeToFileWithCol(file, s) : writeToFile(file, s);
        vm.decVarRef(v);
    }
    return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(print, 1, true,
           "  fn(args...) -> Int\n"
           "Prints each of `args` on `stdout` and returns the number of characters printed.")
{
    ssize_t count = printBase(vm, loc, stdout, {args.begin() + 1, args.end()}, false);
    if(count < 0) return nullptr;
    return vm.makeVar<VarInt>(loc, count);
}

FERAL_FUNC(println, 0, true,
           "  fn(args...) -> Int\n"
           "Prints each of `args` on `stdout`, as well as a newline character at the end.\n"
           "If there are no `args`, just the newline character is printed.\n"
           "Returns the number of printed characters, including the newline character.")
{
    ssize_t count = printBase(vm, loc, stdout, {args.begin() + 1, args.end()}, false);
    if(count < 0) return nullptr;
    count += writeToFile(stdout, "\n");
    return vm.makeVar<VarInt>(loc, count);
}

FERAL_FUNC(cprint, 1, true,
           "  fn(args...) -> Int\n"
           "Same as `print` but also applies terminal color codes on the output.\n"
           "The color shorthands are:\n"
           "  {0}  => Resets any previous colors\n"
           "  {r}  => Red\n"
           "  {g}  => Green\n"
           "  {y}  => Yellow\n"
           "  {b}  => Blue\n"
           "  {m}  => Magenta\n"
           "  {c}  => Cyan\n"
           "  {w}  => White\n"
           "  {br} => Bright Red\n"
           "  {bg} => Bright Green\n"
           "  {by} => Bright Yellow\n"
           "  {bb} => Bright Blue\n"
           "  {bm} => Bright Magenta\n"
           "  {bc} => Bright Cyan\n"
           "  {bw} => Bright White")
{
    ssize_t count = printBase(vm, loc, stdout, {args.begin() + 1, args.end()}, true);
    if(count < 0) return nullptr;
    return vm.makeVar<VarInt>(loc, count);
}

FERAL_FUNC(cprintln, 0, true,
           "  fn(args...) -> Int\n"
           "Same as `println` but also applies terminal color codes on the output like `cprint`.")
{
    ssize_t count = printBase(vm, loc, stdout, {args.begin() + 1, args.end()}, true);
    if(count < 0) return nullptr;
    count += writeToFile(stdout, "\n");
    return vm.makeVar<VarInt>(loc, count);
}

FERAL_FUNC(eprint, 1, true,
           "  fn(args...) -> Int\n"
           "Same as `print` but writes on `stderr` instead of `stdout`.")
{
    ssize_t count = printBase(vm, loc, stderr, {args.begin() + 1, args.end()}, false);
    if(count < 0) return nullptr;
    return vm.makeVar<VarInt>(loc, count);
}

FERAL_FUNC(eprintln, 0, true,
           "  fn(args...) -> Int\n"
           "Same as `println` but writes on `stderr` instead of `stdout`.")
{
    ssize_t count = printBase(vm, loc, stderr, {args.begin() + 1, args.end()}, false);
    if(count < 0) return nullptr;
    count += writeToFile(stderr, "\n");
    return vm.makeVar<VarInt>(loc, count);
}

FERAL_FUNC(ecprint, 1, true,
           "  fn(args...) -> Int\n"
           "Same as `cprint` but writes on `stderr` instead of `stdout`.")
{
    ssize_t count = printBase(vm, loc, stderr, {args.begin() + 1, args.end()}, true);
    if(count < 0) return nullptr;
    return vm.makeVar<VarInt>(loc, count);
}

FERAL_FUNC(ecprintln, 0, true,
           "  fn(args...) -> Int\n"
           "Same as `cprintln` but writes on `stderr` instead of `stdout`.")
{
    ssize_t count = printBase(vm, loc, stderr, {args.begin() + 1, args.end()}, true);
    if(count < 0) return nullptr;
    count += writeToFile(stderr, "\n");
    return vm.makeVar<VarInt>(loc, count);
}

FERAL_FUNC(fprint, 1, true,
           "  fn(file, args...) -> Int\n"
           "Same as `print` but accepts a File `file` to write to.")
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
    FILE *f       = as<VarFile>(args[1])->getFile();
    ssize_t count = printBase(vm, loc, f, {args.begin() + 2, args.end()}, false);
    if(count < 0) return nullptr;
    return vm.makeVar<VarInt>(loc, count);
}

FERAL_FUNC(fprintln, 0, true,
           "  fn(file, args...) -> Int\n"
           "Same as `println` but accepts a File `file` to write to.")
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
    FILE *f       = as<VarFile>(args[1])->getFile();
    ssize_t count = printBase(vm, loc, f, {args.begin() + 2, args.end()}, false);
    if(count < 0) return nullptr;
    count += writeToFile(f, "\n");
    return vm.makeVar<VarInt>(loc, count);
}

FERAL_FUNC(fcprint, 1, true,
           "  fn(file, args...) -> Int\n"
           "Same as `cprint` but accepts a File `file` to write to.")
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
    FILE *f       = as<VarFile>(args[1])->getFile();
    ssize_t count = printBase(vm, loc, f, {args.begin() + 2, args.end()}, true);
    if(count < 0) return nullptr;
    return vm.makeVar<VarInt>(loc, count);
}

FERAL_FUNC(fcprintln, 0, true,
           "  fn(file, args...) -> Int\n"
           "Same as `cprintln` but accepts a File `file` to write to.")
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
    FILE *f       = as<VarFile>(args[1])->getFile();
    ssize_t count = printBase(vm, loc, f, {args.begin() + 2, args.end()}, true);
    if(count < 0) return nullptr;
    count += writeToFile(f, "\n");
    return vm.makeVar<VarInt>(loc, count);
}

FERAL_FUNC(scan, 0, false,
           "  fn() -> Str\n"
           "Reads input line from `stdin` and returns it as a string.")
{
    VarStr *res = vm.makeVar<VarStr>(loc, "");
    std::getline(std::cin, res->getVal());

    if(!res->getVal().empty() && res->getVal().back() == '\r') res->getVal().pop_back();
    if(!res->getVal().empty() && res->getVal().back() == '\n') res->getVal().pop_back();

    return res;
}

FERAL_FUNC(scanEOF, 0, false,
           "  fn() -> Str\n"
           "Reads input until EOF character from `stdin` and returns it as a string.")
{
    String line;
    VarStr *res = vm.makeVar<VarStr>(loc, "");
    while(std::getline(std::cin, line)) res->getVal() += line;

    if(!res->getVal().empty() && (res->getVal().back() == '\r' || res->getVal().back() == '\n'))
        res->getVal().pop_back();

    return res;
}

FERAL_FUNC(fflush, 1, false,
           "  fn(file) -> Nil\n"
           "Flush the `file`, writing any pending data to it.")
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
    ::fflush(as<VarFile>(args[1])->getFile());
    return vm.getNil();
}

FERAL_FUNC(readChar, 1, false,
           "  fn(file) -> Str\n"
           "Reads a character from the file descriptor `file` and returns it as a string.")
{
    if(!args[1]->is<VarInt>()) {
        vm.fail(args[1]->getLoc(), "expected an integer argument for file descriptor, found: ",
                vm.getTypeName(args[1]));
        return nullptr;
    }

    int fd  = as<VarInt>(args[1])->getVal();
    char c  = 0;
    int res = read(fd, &c, 1);
    if(res <= 0) {
        vm.fail(loc, "failed to read char");
        return nullptr;
    }
    return vm.makeVar<VarStr>(loc, c);
}

INIT_DLL(IO)
{
    VarModule *mod = vm.getCurrModule();

    mod->addNativeFn(vm, "print", print);
    mod->addNativeFn(vm, "println", println);
    mod->addNativeFn(vm, "cprint", cprint);
    mod->addNativeFn(vm, "cprintln", cprintln);
    mod->addNativeFn(vm, "eprint", eprint);
    mod->addNativeFn(vm, "eprintln", eprintln);
    mod->addNativeFn(vm, "ecprint", ecprint);
    mod->addNativeFn(vm, "ecprintln", ecprintln);
    mod->addNativeFn(vm, "fprint", fprint);
    mod->addNativeFn(vm, "fprintln", fprintln);
    mod->addNativeFn(vm, "fcprint", fcprint);
    mod->addNativeFn(vm, "fcprintln", fcprintln);
    mod->addNativeFn(vm, "scan", scan);
    mod->addNativeFn(vm, "scanEOF", scanEOF);
    mod->addNativeFn(vm, "fflush", fflush);
    mod->addNativeFn(vm, "readChar", readChar);

    // stdin, stdout, and stderr cannot be owned by a VarFile
    mod->addNativeVar(vm, "stdin", "The standard input stream.",
                      vm.makeVar<VarFile>(loc, stdin, "r", false));
    mod->addNativeVar(vm, "stdout", "The standard output stream.",
                      vm.makeVar<VarFile>(loc, stdout, "w", false));
    mod->addNativeVar(vm, "stderr", "The standard error stream.",
                      vm.makeVar<VarFile>(loc, stderr, "w", false));
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