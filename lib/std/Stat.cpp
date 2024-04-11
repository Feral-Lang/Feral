#include <sys/stat.h> // stat()

#include "std/StructType.hpp"
#include "VM/Interpreter.hpp"

#if defined(FER_OS_WINDOWS)
#include <errno.h> // errno

// Define S_IS*() macros since they're not present on Windows
#if defined(S_IFMT)
#if !defined(S_ISREG) && defined(S_IFREG)
#define S_ISREG(m) (((m)&S_IFMT) == S_IFREG)
#endif
#if !defined(S_ISDIR) && defined(S_IFDIR)
#define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
#endif
#if !defined(S_ISCHR) && defined(S_IFCHR)
#define S_ISCHR(m) (((m)&S_IFMT) == S_IFCHR)
#endif
#endif
#else
#include <sys/errno.h> // errno
#endif

namespace fer
{

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *statNative(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		const Map<String, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStruct>()) {
		vm.fail(args[1]->getLoc(),
			"expected a struct (of type Stat) as first argument, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
#if defined(FER_OS_WINDOWS)
	static const Array<StringRef, 11> reqdkeys = {
#else
	static const Array<StringRef, 13> reqdkeys = {
#endif
		"dev",
		"ino",
		"mode",
		"nlink",
		"uid",
		"gid",
		"rdev",
		"size",
		"atime",
		"mtime",
		"ctime",
#if !defined(FER_OS_WINDOWS)
		"blksize",
		"blocks"
#endif
	};

	VarStruct *st = as<VarStruct>(args[1]);
	for(auto key : reqdkeys) {
		Var *val = st->getAttr(key);
		if(val == nullptr) {
			vm.fail(args[1]->getLoc(), "expected attribute '", key,
				"' in struct of type Stat (provided invalid struct)");
			return nullptr;
		} else if(!val->is<VarInt>()) {
			vm.fail(args[1]->getLoc(), "expected attribute '", key,
				"' to be of type 'int', found: ", vm.getTypeName(val));
			return nullptr;
		}
	}
	if(!args[2]->is<VarStr>()) {
		vm.fail(args[2]->getLoc(),
			"expected a file name string"
			" parameter as second argument, found: ",
			vm.getTypeName(args[2]));
		return nullptr;
	}
	const String &path = as<VarStr>(args[2])->get();
	struct stat _stat;
	int res = stat(path.c_str(), &_stat);
	if(res != 0) {
		vm.fail(loc, "stat for path '", path, "' failed, error(", std::to_string(errno),
			"): ", strerror(errno));
		return nullptr;
	}

	as<VarInt>(st->getAttr("dev"))->set(_stat.st_dev);
	as<VarInt>(st->getAttr("ino"))->set(_stat.st_ino);
	as<VarInt>(st->getAttr("mode"))->set(_stat.st_mode);
	as<VarInt>(st->getAttr("nlink"))->set(_stat.st_nlink);
	as<VarInt>(st->getAttr("uid"))->set(_stat.st_uid);
	as<VarInt>(st->getAttr("gid"))->set(_stat.st_gid);
	as<VarInt>(st->getAttr("rdev"))->set(_stat.st_rdev);
	as<VarInt>(st->getAttr("size"))->set(_stat.st_size);
	as<VarInt>(st->getAttr("atime"))->set(_stat.st_atime);
	as<VarInt>(st->getAttr("mtime"))->set(_stat.st_mtime);
	as<VarInt>(st->getAttr("ctime"))->set(_stat.st_ctime);
#if !defined(FER_OS_WINDOWS)
	as<VarInt>(st->getAttr("blksize"))->set(_stat.st_blksize);
	as<VarInt>(st->getAttr("blocks"))->set(_stat.st_blocks);
#endif

	return vm.getNil();
}

Var *statIsReg(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<String, AssnArgData> &assn_args)
{
	VarStruct *st = as<VarStruct>(args[1]);
	int mode      = as<VarInt>(st->getAttr("mode"))->get();
	return S_ISREG(mode) ? vm.getTrue() : vm.getFalse();
}

Var *statIsDir(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<String, AssnArgData> &assn_args)
{
	VarStruct *st = as<VarStruct>(args[1]);
	int mode      = as<VarInt>(st->getAttr("mode"))->get();
	return S_ISDIR(mode) ? vm.getTrue() : vm.getFalse();
}

Var *statIsChr(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<String, AssnArgData> &assn_args)
{
	VarStruct *st = as<VarStruct>(args[1]);
	int mode      = as<VarInt>(st->getAttr("mode"))->get();
	return S_ISCHR(mode) ? vm.getTrue() : vm.getFalse();
}

#if !defined(FER_OS_WINDOWS)
Var *statIsBlk(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<String, AssnArgData> &assn_args)
{
	VarStruct *st = as<VarStruct>(args[1]);
	int mode      = as<VarInt>(st->getAttr("mode"))->get();
	return S_ISBLK(mode) ? vm.getTrue() : vm.getFalse();
}

Var *statIsFifo(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		const Map<String, AssnArgData> &assn_args)
{
	VarStruct *st = as<VarStruct>(args[1]);
	int mode      = as<VarInt>(st->getAttr("mode"))->get();
	return S_ISFIFO(mode) ? vm.getTrue() : vm.getFalse();
}

Var *statIsLnk(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<String, AssnArgData> &assn_args)
{
	VarStruct *st = as<VarStruct>(args[1]);
	int mode      = as<VarInt>(st->getAttr("mode"))->get();
	return S_ISLNK(mode) ? vm.getTrue() : vm.getFalse();
}

Var *statIsSock(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		const Map<String, AssnArgData> &assn_args)
{
	VarStruct *st = as<VarStruct>(args[1]);
	int mode      = as<VarInt>(st->getAttr("mode"))->get();
	return S_ISSOCK(mode) ? vm.getTrue() : vm.getFalse();
}
#endif

INIT_MODULE(Stat)
{
	VarModule *mod = vm.getCurrModule();

	mod->addNativeFn("statNative", statNative, 2);
	mod->addNativeFn("isRegNative", statIsReg, 1);
	mod->addNativeFn("isDirNative", statIsDir, 1);
	mod->addNativeFn("isChrNative", statIsChr, 1);
#if !defined(FER_OS_WINDOWS)
	mod->addNativeFn("isBlkNative", statIsBlk, 1);
	mod->addNativeFn("isFifoNative", statIsFifo, 1);
	mod->addNativeFn("isLnkNative", statIsLnk, 1);
	mod->addNativeFn("isSockNative", statIsSock, 1);
#endif
	return true;
}

} // namespace fer