#include <fcntl.h>

#include "VM/VM.hpp"

namespace fer
{

enum WalkEntry
{
    FILES   = 1 << 0,
    DIRS    = 1 << 1,
    RECURSE = 1 << 2,
};

void getEntriesInternal(VirtualMachine &vm, ModuleLoc loc, const Path &dir, VarVec *v, Regex regex,
                        size_t flags)
{
    Path entry;
    for(const auto &ent : fs::directory_iterator(dir)) {
        if(ent.path() == "." || ent.path() == "..") continue;
        entry.clear();
        entry += ent.path().generic_string();
        if((!(flags & WalkEntry::RECURSE) || !ent.is_directory()) &&
           !std::regex_match(entry.native(), regex))
        {
            continue;
        }
        if(ent.is_directory()) {
            if(flags & WalkEntry::RECURSE) {
                getEntriesInternal(vm, loc, entry, v, regex, flags);
            } else if(flags & WalkEntry::DIRS) {
                v->push(vm, vm.makeVar<VarPath>(loc, entry), true);
            }
            continue;
        }
        if(flags & WalkEntry::FILES || flags & WalkEntry::RECURSE) {
            v->push(vm, vm.makeVar<VarPath>(loc, entry), true);
        }
    }
}

FERAL_FUNC(fsFopen, 3, false, "")
{
    EXPECT(VarPath, args[1], "file path");
    EXPECT(VarStr, args[2], "file open mode");
    EXPECT(VarBool, args[3], "if file should be closed");

    const String &path = as<VarPath>(args[1])->getStr();
    const String &mode = as<VarStr>(args[2])->getVal();
    bool mustClose     = as<VarBool>(args[3])->getVal();
    FILE *file         = fopen(path.c_str(), mode.c_str());
    if(!file) {
        vm.fail(loc, "failed to open file '", path, "' with mode: ", mode);
        return nullptr;
    }
    return vm.makeVar<VarFile>(loc, file, mode, mustClose);
}

FERAL_FUNC(fsWalkDir, 3, false, "")
{
    EXPECT(VarPath, args[1], "directory name");
    EXPECT(VarInt, args[2], "walk mode");
    EXPECT(VarStr, args[3], "path regex");

    const Path &dir = as<VarPath>(args[1])->getVal();
    if(dir.empty()) {
        vm.fail(loc, "empty directory path provided");
        return nullptr;
    }

    size_t flags = as<VarInt>(args[2])->getVal();

    const String &regexstr = as<VarStr>(args[3])->getVal();
    Regex regex(regexstr);

    VarVec *res = vm.makeVar<VarVec>(loc, 0, false);
    getEntriesInternal(vm, loc, dir, res, regex, flags);
    return res;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// FileSystem Functions ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(fsStatus, 1, false,
           "  fn(path) -> Int\n"
           "Returns the status code as per C++'s `std::filesystem::file_type` for `path`.")
{
    EXPECT(VarPath, args[1], "path");
    std::error_code ec;
    int res = (int)fs::symlink_status(as<VarPath>(args[1])->getVal(), ec).type();
    return vm.makeVar<VarInt>(loc, res);
}

FERAL_FUNC(fsMkdir, 1, true,
           "  fn(paths...) -> Nil\n"
           "Creates all directories in `paths`.")
{
    for(size_t i = 1; i < args.size(); ++i) {
        EXPECT(VarPath, args[i], "directory path");
        VarPath *path = as<VarPath>(args[i]);
        if(path->empty()) continue;
        std::error_code ec;
        fs::create_directories(path->getVal(), ec);
        if(ec.value()) {
            vm.fail(loc, "mkdir failed (", ec.value(), "): ", ec.message());
            return nullptr;
        }
    }
    return vm.getNil();
}

FERAL_FUNC(fsMklinkDir, 2, false,
           "  fn(source, destination) -> Nil\n"
           "Creates a symbolic link at `destination` from `source`, where `source` is a directory.")
{
    EXPECT(VarPath, args[1], "source");
    EXPECT(VarPath, args[2], "destination");
    VarPath *src  = as<VarPath>(args[1]);
    VarPath *dest = as<VarPath>(args[2]);
    std::error_code ec;
    fs::create_directory_symlink(src->getVal(), dest->getVal(), ec);
    if(ec.value()) {
        vm.fail(loc, "mklinkDir failed (", ec.value(), "): ", ec.message());
        return nullptr;
    }
    return vm.getNil();
}

FERAL_FUNC(
    fsMklinkFile, 2, false,
    "  fn(source, destination) -> Nil\n"
    "Creates a symbolic link at `destination` from `source`, where `source` is not a directory.")
{
    EXPECT(VarPath, args[1], "source");
    EXPECT(VarPath, args[2], "destination");
    VarPath *src  = as<VarPath>(args[1]);
    VarPath *dest = as<VarPath>(args[2]);
    std::error_code ec;
    fs::create_symlink(src->getVal(), dest->getVal(), ec);
    if(ec.value()) {
        vm.fail(loc, "mklinkFile failed (", ec.value(), "): ", ec.message());
        return nullptr;
    }
    return vm.getNil();
}

FERAL_FUNC(fsRemove, 1, true,
           "  fn(paths...) -> Nil\n"
           "Removes all the given `paths`, will also delete any subfolders/files if present.")
{
    for(size_t i = 1; i < args.size(); ++i) {
        EXPECT(VarPath, args[i], "path to delete");
        VarPath *path = as<VarPath>(args[i]);
        if(path->empty()) continue;
        std::error_code ec;
        fs::remove_all(path->getVal(), ec);
        if(ec.value()) {
            vm.fail(loc, "remove failed (", ec.value(), "): ", ec.message());
            return nullptr;
        }
    }
    return vm.getNil();
}

FERAL_FUNC(fsCopy, 2, true,
           "  fn(sources..., destination) -> Nil\n"
           "Copies all `sources` to `destination`.")
{
    // args must have: nullptr, [src]+, dest
    Var *destArg = args[args.size() - 1];
    EXPECT(VarPath, destArg, "copy destination");
    const Path &dest = as<VarPath>(destArg)->getVal();
    auto opts        = fs::copy_options::update_existing | fs::copy_options::recursive;
    for(size_t i = 1; i < args.size() - 1; ++i) {
        EXPECT(VarPath, args[i], "path to copy");
        const Path &src = as<VarPath>(args[i])->getVal();
        if(src.empty()) continue;
        std::error_code ec;
        fs::copy(src, dest, opts, ec);
        if(ec.value()) {
            vm.fail(loc, "copy failed (", ec.value(), "): ", ec.message());
            return nullptr;
        }
    }
    return vm.getNil();
}

FERAL_FUNC(fsMove, 2, false,
           "  fn(source, destination) -> Nil\n"
           "Move the file/directory from `source` to `destination`.")
{
    EXPECT(VarPath, args[1], "source");
    EXPECT(VarPath, args[2], "destination");
    const Path &from = as<VarPath>(args[1])->getVal();
    const Path &to   = as<VarPath>(args[2])->getVal();
    std::error_code ec;
    fs::rename(from, to, ec);
    if(ec.value()) {
        vm.fail(loc, "rename failed (", ec.value(), "): ", ec.message());
        return nullptr;
    }
    return vm.getNil();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// File Types Functions ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

FERAL_FUNC(pathIsBlock, 1, false,
           "  fn(path) -> Bool\n"
           "Returns `true` if the given `path` is a block device.")
{
    EXPECT(VarPath, args[1], "path");
    std::error_code ec;
    bool res = fs::is_block_file(as<VarPath>(args[1])->getVal(), ec);
    if(ec.value()) {
        vm.fail(loc, "isBlock failed (", ec.value(), "): ", ec.message());
        return nullptr;
    }
    return res ? vm.getTrue() : vm.getFalse();
}

FERAL_FUNC(pathIsChar, 1, false,
           "  fn(path) -> Bool\n"
           "Returns `true` if the given `path` is a character device.")
{
    EXPECT(VarPath, args[1], "path");
    std::error_code ec;
    bool res = fs::is_character_file(as<VarPath>(args[1])->getVal(), ec);
    if(ec.value()) {
        vm.fail(loc, "isChar failed (", ec.value(), "): ", ec.message());
        return nullptr;
    }
    return res ? vm.getTrue() : vm.getFalse();
}

FERAL_FUNC(pathIsDir, 1, false,
           "  fn(path) -> Bool\n"
           "Returns `true` if the given `path` is a directory.")
{
    EXPECT(VarPath, args[1], "path");
    std::error_code ec;
    bool res = fs::is_directory(as<VarPath>(args[1])->getVal(), ec);
    if(ec.value()) {
        vm.fail(loc, "isDir failed (", ec.value(), "): ", ec.message());
        return nullptr;
    }
    return res ? vm.getTrue() : vm.getFalse();
}

FERAL_FUNC(pathIsEmpty, 1, false,
           "  fn(path) -> Bool\n"
           "Returns `true` if the given `path` is an empty file/directory.")
{
    EXPECT(VarPath, args[1], "path");
    std::error_code ec;
    bool res = fs::is_empty(as<VarPath>(args[1])->getVal(), ec);
    if(ec.value()) {
        vm.fail(loc, "isEmpty failed (", ec.value(), "): ", ec.message());
        return nullptr;
    }
    return res ? vm.getTrue() : vm.getFalse();
}

FERAL_FUNC(pathIsFifo, 1, false,
           "  fn(path) -> Bool\n"
           "Returns `true` if the given `path` is a fifo device.")
{
    EXPECT(VarPath, args[1], "path");
    std::error_code ec;
    bool res = fs::is_fifo(as<VarPath>(args[1])->getVal(), ec);
    if(ec.value()) {
        vm.fail(loc, "isFifo failed (", ec.value(), "): ", ec.message());
        return nullptr;
    }
    return res ? vm.getTrue() : vm.getFalse();
}

FERAL_FUNC(pathIsOther, 1, false,
           "  fn(path) -> Bool\n"
           "Returns `true` if the given `path` exists but is neither of regular file / directory / "
           "symlink.")
{
    EXPECT(VarPath, args[1], "path");
    std::error_code ec;
    bool res = fs::is_other(as<VarPath>(args[1])->getVal(), ec);
    if(ec.value()) {
        vm.fail(loc, "isOther failed (", ec.value(), "): ", ec.message());
        return nullptr;
    }
    return res ? vm.getTrue() : vm.getFalse();
}

FERAL_FUNC(pathIsReg, 1, false,
           "  fn(path) -> Bool\n"
           "Returns `true` if the given `path` is a regular file.")
{
    EXPECT(VarPath, args[1], "path");
    std::error_code ec;
    bool res = fs::is_regular_file(as<VarPath>(args[1])->getVal(), ec);
    if(ec.value()) {
        vm.fail(loc, "isReg failed (", ec.value(), "): ", ec.message());
        return nullptr;
    }
    return res ? vm.getTrue() : vm.getFalse();
}

FERAL_FUNC(pathIsSocket, 1, false,
           "  fn(path) -> Bool\n"
           "Returns `true` if the given `path` is a socket device.")
{
    EXPECT(VarPath, args[1], "path");
    std::error_code ec;
    bool res = fs::is_socket(as<VarPath>(args[1])->getVal(), ec);
    if(ec.value()) {
        vm.fail(loc, "isSocket failed (", ec.value(), "): ", ec.message());
        return nullptr;
    }
    return res ? vm.getTrue() : vm.getFalse();
}

FERAL_FUNC(pathIsSymlink, 1, false,
           "  fn(path) -> Bool\n"
           "Returns `true` if the given `path` is a symlink.")
{
    EXPECT(VarPath, args[1], "path");
    std::error_code ec;
    bool res = fs::is_symlink(as<VarPath>(args[1])->getVal(), ec);
    if(ec.value()) {
        vm.fail(loc, "isSymlink failed (", ec.value(), "): ", ec.message());
        return nullptr;
    }
    return res ? vm.getTrue() : vm.getFalse();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// File Descriptor Functions //////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: write tests

// equivalent to open(path, O_WRONLY | O_CREAT | O_TRUNC, mode)
FERAL_FUNC(fdCreate, 2, false, "")
{
    EXPECT(VarPath, args[1], "path");
    EXPECT(VarInt, args[2], "mode");
    String path = as<VarPath>(args[1])->getStr();
    int res     = creat(path.c_str(), as<VarInt>(args[2])->getVal());
    if(res < 0) {
        vm.fail(loc, "failed to create file: '", path, "', error: ", strerror(errno));
        return nullptr;
    }
    return vm.makeVar<VarInt>(loc, res);
}

FERAL_FUNC(fdOpen, 2, false, "")
{
    EXPECT(VarPath, args[1], "path");
    EXPECT(VarInt, args[2], "open flags");
    String path = as<VarPath>(args[1])->getStr();
    int res     = open(path.c_str(), as<VarInt>(args[2])->getVal());
    if(res < 0) {
        vm.fail(loc, "failed to open file: '", path, "', error: ", strerror(errno));
        return nullptr;
    }
    return vm.makeVar<VarInt>(loc, res);
}

FERAL_FUNC(fdRead, 2, false,
           "  fn(fileDescriptor, bytebufferData) -> Int\n"
           "Given a `fileDescriptor`, read up to capacity of `bytebufferData` bytes from it.\n"
           "Returns number of read bytes.")
{
    EXPECT(VarInt, args[1], "file descriptor");
    EXPECT(VarBytebuffer, args[2], "the data to write");
    VarBytebuffer *bb = as<VarBytebuffer>(args[2]);
    errno             = 0;
    ssize_t res       = read(as<VarInt>(args[1])->getVal(), bb->getVal(), bb->capacity());
    if(res < 0 || errno != 0) {
        vm.fail(loc, "failed to read from the file descriptor: '",
                std::to_string(as<VarInt>(args[1])->getVal()), "', error: ", strerror(errno));
        return nullptr;
    }
    bb->setLen(res);
    return vm.makeVar<VarInt>(loc, res);
}

FERAL_FUNC(fdWrite, 2, false,
           "  fn(fileDescriptor, bytebufferData | str) -> Int\n"
           "Given a `fileDescriptor`, write `bytebufferData` / `str` to it.")
{
    EXPECT(VarInt, args[1], "file descriptor");
    EXPECT2(VarBytebuffer, VarStr, args[2], "the data to write");
    const void *data = nullptr;
    size_t count     = 0;
    errno            = 0;
    if(args[2]->is<VarBytebuffer>()) {
        VarBytebuffer *bb = as<VarBytebuffer>(args[2]);
        data              = bb->getVal();
        count             = bb->size();
    } else if(args[2]->is<VarStr>()) {
        VarStr *s = as<VarStr>(args[2]);
        data      = s->getVal().data();
        count     = s->getVal().size();
    }
    ssize_t res = write(as<VarInt>(args[1])->getVal(), data, count);
    if(res < 0 || errno != 0) {
        vm.fail(loc, "failed to write to the file descriptor: '",
                std::to_string(as<VarInt>(args[1])->getVal()), "', error: ", strerror(errno));
        return nullptr;
    }
    return vm.makeVar<VarInt>(loc, res);
}

FERAL_FUNC(fdClose, 1, false,
           "  fn(fileDescriptor) -> Int\n"
           "Close the given `fileDescriptor` and return the status.")
{
    EXPECT(VarInt, args[1], "file descriptor");
    int res = close(as<VarInt>(args[1])->getVal());
    if(res < 0) {
        vm.fail(loc, "failed to close the file descriptor: '",
                std::to_string(as<VarInt>(args[1])->getVal()), "', error: ", strerror(errno));
        return nullptr;
    }
    return vm.makeVar<VarInt>(loc, res);
}

INIT_DLL(FS)
{
    // file/filesystem
    vm.addLocal(loc, "fopenNative", fsFopen);
    vm.addLocal(loc, "walkDirNative", fsWalkDir);
    // files and dirs
    vm.addLocal(loc, "status", fsStatus);
    vm.addLocal(loc, "move", fsMove);
    vm.addLocal(loc, "mkdir", fsMkdir);
    vm.addLocal(loc, "mklinkDir", fsMklinkDir);
    vm.addLocal(loc, "mklinkFile", fsMklinkFile);
    vm.addLocal(loc, "remove", fsRemove);
    vm.addLocal(loc, "copy", fsCopy);
    // path/file types
    vm.addLocal(loc, "isBlock", pathIsBlock);
    vm.addLocal(loc, "isChar", pathIsChar);
    vm.addLocal(loc, "isDir", pathIsDir);
    vm.addLocal(loc, "isEmpty", pathIsEmpty);
    vm.addLocal(loc, "isFifo", pathIsFifo);
    vm.addLocal(loc, "isOther", pathIsOther);
    vm.addLocal(loc, "isReg", pathIsReg);
    vm.addLocal(loc, "isSocket", pathIsSocket);
    vm.addLocal(loc, "isSymlink", pathIsSymlink);
    // file descriptor
    vm.addLocal(loc, "fdCreateNative", fdCreate);
    vm.addLocal(loc, "fdOpenNative", fdOpen);
    vm.addLocal(loc, "fdRead", fdRead);
    vm.addLocal(loc, "fdWrite", fdWrite);
    vm.addLocal(loc, "fdClose", fdClose);

    // constants (for file/filesystem)
    // std::filesystem::file_type
    vm.makeLocal<VarInt>(loc, "TYPE_NONE", "", (int)fs::file_type::none);
    vm.makeLocal<VarInt>(loc, "TYPE_NOT_FOUND", "", (int)fs::file_type::not_found);
    vm.makeLocal<VarInt>(loc, "TYPE_REG", "", (int)fs::file_type::regular);
    vm.makeLocal<VarInt>(loc, "TYPE_DIR", "", (int)fs::file_type::directory);
    vm.makeLocal<VarInt>(loc, "TYPE_SYMLINK", "", (int)fs::file_type::symlink);
    vm.makeLocal<VarInt>(loc, "TYPE_BLOCK", "", (int)fs::file_type::block);
    vm.makeLocal<VarInt>(loc, "TYPE_CHAR", "", (int)fs::file_type::character);
    vm.makeLocal<VarInt>(loc, "TYPE_FIFO", "", (int)fs::file_type::fifo);
    vm.makeLocal<VarInt>(loc, "TYPE_SOCKET", "", (int)fs::file_type::socket);
    vm.makeLocal<VarInt>(loc, "TYPE_UNKNOWN", "", (int)fs::file_type::unknown);
    // stdin, stdout, stderr file descriptors
    vm.makeLocal<VarInt>(loc, "stdin", "The standard input stream.", STDIN_FILENO);
    vm.makeLocal<VarInt>(loc, "stdout", "The standard output stream.", STDOUT_FILENO);
    vm.makeLocal<VarInt>(loc, "stderr", "The standard error stream.", STDERR_FILENO);
    // fs.walkdir()
    vm.makeLocal<VarInt>(loc, "WALK_FILES", "", WalkEntry::FILES);
    vm.makeLocal<VarInt>(loc, "WALK_DIRS", "", WalkEntry::DIRS);
    vm.makeLocal<VarInt>(loc, "WALK_RECURSE", "", WalkEntry::RECURSE);
    // <file>.seek()
    vm.makeLocal<VarInt>(loc, "SEEK_SET", "", SEEK_SET);
    vm.makeLocal<VarInt>(loc, "SEEK_CUR", "", SEEK_CUR);
    vm.makeLocal<VarInt>(loc, "SEEK_END", "", SEEK_END);
    // file descriptor flags
    vm.makeLocal<VarInt>(loc, "O_RDONLY", "", O_RDONLY);
    vm.makeLocal<VarInt>(loc, "O_WRONLY", "", O_WRONLY);
    vm.makeLocal<VarInt>(loc, "O_RDWR", "", O_RDWR);
    vm.makeLocal<VarInt>(loc, "O_APPEND", "", O_APPEND);
    vm.makeLocal<VarInt>(loc, "O_CREAT", "", O_CREAT);
#if defined(CORE_OS_LINUX) || defined(CORE_OS_APPLE)
    vm.makeLocal<VarInt>(loc, "O_DSYNC", "", O_DSYNC);
#endif
    vm.makeLocal<VarInt>(loc, "O_EXCL", "", O_EXCL);
#if !defined(CORE_OS_WINDOWS)
    vm.makeLocal<VarInt>(loc, "O_NOCTTY", "", O_NOCTTY);
    vm.makeLocal<VarInt>(loc, "O_NONBLOCK", "", O_NONBLOCK);
    vm.makeLocal<VarInt>(loc, "O_SYNC", "", O_SYNC);
#endif
#if defined(CORE_OS_LINUX)
    vm.makeLocal<VarInt>(loc, "O_RSYNC", "", O_RSYNC);
#endif
    vm.makeLocal<VarInt>(loc, "O_TRUNC", "", O_TRUNC);

    return true;
}

} // namespace fer