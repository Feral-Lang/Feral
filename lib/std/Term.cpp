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

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(isTTYNative, 1, false,
           "  fn(fd = fs.stdin) -> Bool\n"
           "Returns `true` if the file descriptor `fd` is a TTY.")
{
    EXPECT(VarInt, args[1], "file descriptor");
    bool res =
#if defined(CORE_OS_WINDOWS)
        _isatty(as<VarInt>(args[1])->getVal());
#else
        isatty(as<VarInt>(args[1])->getVal());
#endif
    return res ? vm.getTrue() : vm.getFalse();
}

FERAL_FUNC(hideInputNative, 1, false,
           "  fn(shouldHide) -> Nil\n"
           "Input on the terminal will not be showed if `shouldHide` is `true`.")
{
    EXPECT(VarBool, args[1], "should hide");
    bool hidden = as<VarBool>(args[1])->getVal();
#if defined(CORE_OS_WINDOWS)
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    if(!GetConsoleMode(hStdin, &mode)) {
        vm.fail(loc, "GetConsoleMode failed");
        return nullptr;
    }
    if(hidden) {
        mode &= ~ENABLE_ECHO_INPUT;
    } else {
        mode |= ENABLE_ECHO_INPUT;
    }
    if(!SetConsoleMode(hStdin, mode)) {
        vm.fail(loc, "SetConsoleMode failed");
        return nullptr;
    }
#else
    struct termios tty;
    if(tcgetattr(STDIN_FILENO, &tty) != 0) {
        vm.fail(loc, "tcgetattr failed");
        return nullptr;
    }
    if(hidden) {
        tty.c_lflag &= static_cast<decltype(tty.c_lflag)>(~ECHO);
    } else {
        tty.c_lflag |= ECHO;
    }
    if(tcsetattr(STDIN_FILENO, TCSANOW, &tty) != 0) {
        vm.fail(loc, "tcsetattr failed");
        return nullptr;
    }
#endif
    return vm.getNil();
}

FERAL_FUNC(getANSISeq, 1, false,
           "  fn(code) -> Str\n"
           "Returns the ANSI escape sequence for the `code` or empty string if none found.\n"
           "`code` can be:\n"
           "- anchor:link: creates a hyper-link named `anchor`, with `link` as the target\n"
           "- 0: reset formatting\n"
           "- r: red color\n"
           "- g: green color\n"
           "- y: yellow color\n"
           "- b: blue color\n"
           "- m: magenta color\n"
           "- c: cyan color\n"
           "- w: white color\n"
           "- br: bold red color\n"
           "- bg: bold green color\n"
           "- by: bold yellow color\n"
           "- bb: bold blue color\n"
           "- bm: bold magenta color\n"
           "- bc: bold cyan color\n"
           "- bw: bold white color")
{
    EXPECT(VarStr, args[1], "code");
    StringRef code = as<VarStr>(args[1])->getVal();

    auto colonPos = code.find(':');
    if(colonPos != StringRef::npos) {
        StringRef anchor = code.substr(0, colonPos);
        StringRef link   = code.substr(colonPos + 1);
        VarStr *res      = vm.makeVar<VarStr>(loc, "\033]8;;");
        res->getVal() += link;
        res->getVal() += "\033\\";
        res->getVal() += anchor;
        // Move cursor backward and then forward (\033[1D\033[1C) is here because
        // `\\` (before that sequence) at the end would cause havoc in string.fmt().
        res->getVal() += "\033]8;;\033\\\033[1D\033[1C";
        return res;
    }

    // reset
    if(code == "0") return vm.makeVar<VarStr>(loc, "\033[0m");

    // regular
    else if(code == "r") return vm.makeVar<VarStr>(loc, "\033[0;31m");
    else if(code == "g") return vm.makeVar<VarStr>(loc, "\033[0;32m");
    else if(code == "y") return vm.makeVar<VarStr>(loc, "\033[0;33m");
    else if(code == "b") return vm.makeVar<VarStr>(loc, "\033[0;34m");
    else if(code == "m") return vm.makeVar<VarStr>(loc, "\033[0;35m");
    else if(code == "c") return vm.makeVar<VarStr>(loc, "\033[0;36m");
    else if(code == "w") return vm.makeVar<VarStr>(loc, "\033[0;37m");

    // bold
    else if(code == "br") return vm.makeVar<VarStr>(loc, "\033[1;31m");
    else if(code == "bg") return vm.makeVar<VarStr>(loc, "\033[1;32m");
    else if(code == "by") return vm.makeVar<VarStr>(loc, "\033[1;33m");
    else if(code == "bb") return vm.makeVar<VarStr>(loc, "\033[1;34m");
    else if(code == "bm") return vm.makeVar<VarStr>(loc, "\033[1;35m");
    else if(code == "bc") return vm.makeVar<VarStr>(loc, "\033[1;36m");
    else if(code == "bw") return vm.makeVar<VarStr>(loc, "\033[1;37m");

    return vm.makeVar<VarStr>(loc, "");
}

INIT_DLL(Term)
{
    vm.addLocal(loc, "isTTYNative", isTTYNative);
    vm.addLocal(loc, "hideInputNative", hideInputNative);
    vm.addLocal(loc, "getANSISeq", getANSISeq);
    return true;
}

} // namespace fer