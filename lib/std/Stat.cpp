#include <sys/errno.h> // errno
#include <sys/stat.h>  // stat()

#include "FS.hpp"
#include "std/StructType.hpp"
#include "VM/Interpreter.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *statNative(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		const Map<StringRef, AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStruct>()) {
		vm.fail(args[1]->getLoc(),
			{"expected a struct (of type Stat) as first argument, found: ",
			 vm.getTypeName(args[1])});
		return nullptr;
	}
	static const Array<StringRef, 13> reqdkeys = {"dev",   "ino",	  "mode",  "nlink", "uid",
						      "gid",   "rdev",	  "size",  "atime", "mtime",
						      "ctime", "blksize", "blocks"};

	VarStruct *st = as<VarStruct>(args[1]);
	for(auto key : reqdkeys) {
		Var *val = st->getAttr(key);
		if(val == nullptr) {
			vm.fail(args[1]->getLoc(), {"expected attribute '", key,
						    "' in struct of type Stat (provided "
						    "invalid struct)"});
			return nullptr;
		} else if(!val->is<VarInt>()) {
			vm.fail(args[1]->getLoc(),
				{"expected attribute '", key,
				 "' to be of type 'int', found: ", vm.getTypeName(val)});
			return nullptr;
		}
	}
	if(!args[2]->is<VarStr>() && !args[2]->is<VarStrRef>()) {
		vm.fail(args[2]->getLoc(), {"expected a file name string/stringref"
					    " parameter as second argument, found: ",
					    vm.getTypeName(args[2])});
		return nullptr;
	}
	struct stat _stat;
	StringRef path;
	int res = 0;
	if(args[2]->is<VarStr>()) {
		path = as<VarStr>(args[2])->get();
		res  = stat(as<VarStr>(args[2])->get().c_str(), &_stat);
	} else if(args[2]->is<VarStrRef>()) {
		char _path[MAX_PATH_CHARS];
		path = as<VarStrRef>(args[2])->get();
		strncpy(_path, path.data(), path.size());
		_path[path.size()] = '\0';
		res		   = stat(_path, &_stat);
	}
	if(res != 0) {
		vm.fail(loc, {"stat for path '", path, "' failed, error(", std::to_string(errno),
			      "): ", strerror(errno)});
		return nullptr;
	}

	mpz_set_si(as<VarInt>(st->getAttr("dev"))->get(), _stat.st_dev);
	mpz_set_si(as<VarInt>(st->getAttr("ino"))->get(), _stat.st_ino);
	mpz_set_si(as<VarInt>(st->getAttr("mode"))->get(), _stat.st_mode);
	mpz_set_si(as<VarInt>(st->getAttr("nlink"))->get(), _stat.st_nlink);
	mpz_set_si(as<VarInt>(st->getAttr("uid"))->get(), _stat.st_uid);
	mpz_set_si(as<VarInt>(st->getAttr("gid"))->get(), _stat.st_gid);
	mpz_set_si(as<VarInt>(st->getAttr("rdev"))->get(), _stat.st_rdev);
	mpz_set_si(as<VarInt>(st->getAttr("size"))->get(), _stat.st_size);
	mpz_set_si(as<VarInt>(st->getAttr("atime"))->get(), _stat.st_atime);
	mpz_set_si(as<VarInt>(st->getAttr("mtime"))->get(), _stat.st_mtime);
	mpz_set_si(as<VarInt>(st->getAttr("ctime"))->get(), _stat.st_ctime);
	mpz_set_si(as<VarInt>(st->getAttr("blksize"))->get(), _stat.st_blksize);
	mpz_set_si(as<VarInt>(st->getAttr("blocks"))->get(), _stat.st_blocks);

	return vm.getNil();
}

Var *statIsReg(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<StringRef, AssnArgData> &assn_args)
{
	VarStruct *st = as<VarStruct>(args[1]);
	int mode      = mpz_get_si(as<VarInt>(st->getAttr("mode"))->getSrc());
	return S_ISREG(mode) ? vm.getTrue() : vm.getFalse();
}

Var *statIsDir(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<StringRef, AssnArgData> &assn_args)
{
	VarStruct *st = as<VarStruct>(args[1]);
	int mode      = mpz_get_si(as<VarInt>(st->getAttr("mode"))->getSrc());
	return S_ISDIR(mode) ? vm.getTrue() : vm.getFalse();
}

Var *statIsChr(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<StringRef, AssnArgData> &assn_args)
{
	VarStruct *st = as<VarStruct>(args[1]);
	int mode      = mpz_get_si(as<VarInt>(st->getAttr("mode"))->getSrc());
	return S_ISCHR(mode) ? vm.getTrue() : vm.getFalse();
}

Var *statIsBlk(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<StringRef, AssnArgData> &assn_args)
{
	VarStruct *st = as<VarStruct>(args[1]);
	int mode      = mpz_get_si(as<VarInt>(st->getAttr("mode"))->getSrc());
	return S_ISBLK(mode) ? vm.getTrue() : vm.getFalse();
}

Var *statIsFifo(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		const Map<StringRef, AssnArgData> &assn_args)
{
	VarStruct *st = as<VarStruct>(args[1]);
	int mode      = mpz_get_si(as<VarInt>(st->getAttr("mode"))->getSrc());
	return S_ISFIFO(mode) ? vm.getTrue() : vm.getFalse();
}

Var *statIsLnk(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const Map<StringRef, AssnArgData> &assn_args)
{
	VarStruct *st = as<VarStruct>(args[1]);
	int mode      = mpz_get_si(as<VarInt>(st->getAttr("mode"))->getSrc());
	return S_ISLNK(mode) ? vm.getTrue() : vm.getFalse();
}

Var *statIsSock(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		const Map<StringRef, AssnArgData> &assn_args)
{
	VarStruct *st = as<VarStruct>(args[1]);
	int mode      = mpz_get_si(as<VarInt>(st->getAttr("mode"))->getSrc());
	return S_ISSOCK(mode) ? vm.getTrue() : vm.getFalse();
}

INIT_MODULE(Stat)
{
	VarModule *mod = vm.getCurrModule();

	mod->addNativeFn("statNative", statNative, 2);
	mod->addNativeFn("isRegNative", statIsReg, 1);
	mod->addNativeFn("isDirNative", statIsDir, 1);
	mod->addNativeFn("isChrNative", statIsChr, 1);
	mod->addNativeFn("isBlkNative", statIsBlk, 1);
	mod->addNativeFn("isFifoNative", statIsFifo, 1);
	mod->addNativeFn("isLnkNative", statIsLnk, 1);
	mod->addNativeFn("isSockNative", statIsSock, 1);
	return true;
}