#include <iostream>

#include "VM/VM.hpp"

#if defined(CORE_OS_WINDOWS)
#include <io.h>
#include <Windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace fer
{

inline ssize_t writeToFile(FILE *file, StringRef data)
{
    return fwrite(data.data(), sizeof(char), data.size(), file);
}
ssize_t printBase(VirtualMachine &vm, ModuleLoc loc, FILE *file, Span<Var *> args)
{
    ssize_t count = 0;
    for(auto &a : args) {
        if(a->is<VarStr>()) {
            const String &s = as<VarStr>(a)->getVal();
            count += writeToFile(file, s);
            continue;
        }
        Var *v = nullptr;
        Array<Var *, 1> tmp{a};
        if(!vm.callVarAndExpect<VarStr>(loc, "str", v, tmp, {})) return -1;
        const String &s = as<VarStr>(v)->getVal();
        count += writeToFile(file, s);
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
    ssize_t count = printBase(vm, loc, stdout, {args.begin() + 1, args.end()});
    if(count < 0) return nullptr;
    return vm.makeVar<VarInt>(loc, count);
}

FERAL_FUNC(println, 0, true,
           "  fn(args...) -> Int\n"
           "Prints each of `args` on `stdout`, as well as a newline character at the end.\n"
           "If there are no `args`, just the newline character is printed.\n"
           "Returns the number of printed characters, including the newline character.")
{
    ssize_t count = printBase(vm, loc, stdout, {args.begin() + 1, args.end()});
    if(count < 0) return nullptr;
    count += writeToFile(stdout, "\n");
    return vm.makeVar<VarInt>(loc, count);
}

FERAL_FUNC(eprint, 1, true,
           "  fn(args...) -> Int\n"
           "Same as `print` but writes on `stderr` instead of `stdout`.")
{
    ssize_t count = printBase(vm, loc, stderr, {args.begin() + 1, args.end()});
    if(count < 0) return nullptr;
    return vm.makeVar<VarInt>(loc, count);
}

FERAL_FUNC(eprintln, 0, true,
           "  fn(args...) -> Int\n"
           "Same as `println` but writes on `stderr` instead of `stdout`.")
{
    ssize_t count = printBase(vm, loc, stderr, {args.begin() + 1, args.end()});
    if(count < 0) return nullptr;
    count += writeToFile(stderr, "\n");
    return vm.makeVar<VarInt>(loc, count);
}

FERAL_FUNC(fprint, 1, true,
           "  fn(file, args...) -> Int\n"
           "Same as `print` but accepts a File `file` to write to.")
{
    EXPECT(VarFile, args[1], "file");
    if(as<VarFile>(args[1])->getFile() == nullptr) {
        vm.fail(args[1]->getLoc(), "file has probably been closed already",
                vm.getTypeName(args[1]));
        return nullptr;
    }
    FILE *f       = as<VarFile>(args[1])->getFile();
    ssize_t count = printBase(vm, loc, f, {args.begin() + 2, args.end()});
    if(count < 0) return nullptr;
    return vm.makeVar<VarInt>(loc, count);
}

FERAL_FUNC(fprintln, 0, true,
           "  fn(file, args...) -> Int\n"
           "Same as `println` but accepts a File `file` to write to.")
{
    EXPECT(VarFile, args[1], "file");
    if(as<VarFile>(args[1])->getFile() == nullptr) {
        vm.fail(args[1]->getLoc(), "file has probably been closed already",
                vm.getTypeName(args[1]));
        return nullptr;
    }
    FILE *f       = as<VarFile>(args[1])->getFile();
    ssize_t count = printBase(vm, loc, f, {args.begin() + 2, args.end()});
    if(count < 0) return nullptr;
    count += writeToFile(f, "\n");
    return vm.makeVar<VarInt>(loc, count);
}

FERAL_FUNC(scanNative, 0, false, "")
{
    EXPECT(VarBool, args[1], "hide input");
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
    EXPECT(VarFile, args[1], "file");
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
    EXPECT(VarInt, args[1], "file descriptor");
    int fd  = as<VarInt>(args[1])->getVal();
    char c  = 0;
    int res = read(fd, &c, 1);
    if(res <= 0) {
        vm.fail(loc, "failed to read char");
        return nullptr;
    }
    return vm.makeVar<VarStr>(loc, c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// Utility Functions //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

INIT_DLL(IO)
{
    vm.addLocal(loc, "print", print);
    vm.addLocal(loc, "println", println);
    vm.addLocal(loc, "eprint", eprint);
    vm.addLocal(loc, "eprintln", eprintln);
    vm.addLocal(loc, "fprint", fprint);
    vm.addLocal(loc, "fprintln", fprintln);
    vm.addLocal(loc, "scanNative", scanNative);
    vm.addLocal(loc, "scanEOF", scanEOF);
    vm.addLocal(loc, "fflush", fflush);
    vm.addLocal(loc, "readChar", readChar);

    // stdin, stdout, and stderr cannot be owned by a VarFile
    vm.makeLocal<VarFile>(loc, "stdin", "The standard input stream.", stdin, "r", false);
    vm.makeLocal<VarFile>(loc, "stdout", "The standard output stream.", stdout, "w", false);
    vm.makeLocal<VarFile>(loc, "stderr", "The standard error stream.", stderr, "w", false);
    return true;
}

} // namespace fer