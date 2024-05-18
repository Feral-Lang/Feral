#include <fcntl.h>
#include <filesystem>
#include <regex>

#include "std/BytebufferType.hpp"
#include "std/FSType.hpp"
#include "VM/Interpreter.hpp"

#if defined(FER_OS_WINDOWS)
#include <io.h>
#else
#include <dirent.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fer
{

enum WalkEntry
{
	FILES	= 1 << 0,
	DIRS	= 1 << 1,
	RECURSE = 1 << 2,
};

void getEntriesInternal(Interpreter &vm, const ModuleLoc *loc, const String &dirstr, VarVec *v,
			Regex regex, size_t flags);

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Functions ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

Var *fsExists(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for path, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	return fs::exists(as<VarStr>(args[1])->get()) ? vm.getTrue() : vm.getFalse();
}

Var *fsOpen(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for path, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for file open mode, found: ",
			vm.getTypeName(args[2]));
		return nullptr;
	}
	const String &filename = as<VarStr>(args[1])->get();
	const String &mode     = as<VarStr>(args[2])->get();
	FILE *file	       = fopen(filename.c_str(), mode.c_str());
	if(!file) {
		vm.fail(loc, "failed to open file '", filename, "' with mode: ", mode);
		return nullptr;
	}
	return vm.makeVar<VarFile>(loc, file, mode);
}

Var *fsWalkDir(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for directory name, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarInt>()) {
		vm.fail(loc,
			"expected int argument "
			"for walk mode, found: ",
			vm.getTypeName(args[2]));
		return nullptr;
	}
	if(!args[3]->is<VarStr>()) {
		vm.fail(
		loc, "expected string argument for file regex, found: ", vm.getTypeName(args[3]));
		return nullptr;
	}
	String dirstr = as<VarStr>(args[1])->get();
	size_t flags  = as<VarInt>(args[2])->get();

	if(dirstr.empty()) {
		vm.fail(loc, "empty directory path provided");
		return nullptr;
	}

	const String &regexstr = as<VarStr>(args[3])->get();
	Regex regex(regexstr);

	VarVec *res = vm.makeVar<VarVec>(loc, 0, false);
	getEntriesInternal(vm, loc, dirstr, res, regex, flags);
	return res;
}

Var *fileReopen(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		const StringMap<AssnArgData> &assn_args)
{
	VarFile *filev = as<VarFile>(args[0]);
	if(filev->getFile() && filev->isOwner()) fclose(filev->getFile());

	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for path, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for file open mode, found: ",
			vm.getTypeName(args[2]));
		return nullptr;
	}
	const String &filename = as<VarStr>(args[1])->get();
	const String &mode     = as<VarStr>(args[2])->get();
	filev->getFile()       = fopen(filename.c_str(), mode.c_str());
	if(!filev->getFile()) {
		vm.fail(loc, "failed to open file '", filename, "' with mode: ", mode);
		filev->setMode("");
		filev->setOwner(true);
		return nullptr;
	}
	return args[0];
}

Var *fileLines(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	       const StringMap<AssnArgData> &assn_args)
{
	FILE *const file = as<VarFile>(args[0])->getFile();
	char *lineptr	 = NULL;
	size_t len	 = 0;
	ssize_t read	 = 0;

	VarVec *res = vm.makeVar<VarVec>(loc, 0, false);
	while((read = getline(&lineptr, &len, file)) != -1) {
		VarStr *s = vm.makeVarWithRef<VarStr>(loc, lineptr);
		while(!s->get().empty() && (s->get().back() == '\n' || s->get().back() == '\r')) {
			s->get().pop_back();
		}
		res->push(s);
		free(lineptr);
		lineptr = NULL;
	}
	if(lineptr) free(lineptr);
	fseek(file, 0, SEEK_SET);
	return res;
}

Var *fileSeek(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	FILE *const file = as<VarFile>(args[0])->getFile();
	if(!args[1]->is<VarInt>()) {
		vm.fail(loc, "expected int argument for file seek position, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarInt>()) {
		vm.fail(loc, "expected int argument for file seek origin, found: ",
			vm.getTypeName(args[2]));
		return nullptr;
	}
	long pos   = as<VarInt>(args[1])->get();
	int origin = as<VarInt>(args[2])->get();
	return vm.makeVar<VarInt>(loc, fseek(file, pos, origin));
}

Var *fileEachLine(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		  const StringMap<AssnArgData> &assn_args)
{
	return vm.makeVar<VarFileIterator>(loc, as<VarFile>(args[0]));
}

Var *fileIteratorNext(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		      const StringMap<AssnArgData> &assn_args)
{
	VarFileIterator *it = as<VarFileIterator>(args[0]);
	VarStr *res	    = vm.makeVar<VarStr>(loc, "");
	if(!it->next(res)) {
		vm.unmakeVar(res);
		return vm.getNil();
	}
	return res;
}

Var *fileReadAll(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		 const StringMap<AssnArgData> &assn_args)
{
	FILE *const file = as<VarFile>(args[0])->getFile();
	char *lineptr	 = NULL;
	size_t len	 = 0;
	ssize_t read	 = 0;

	VarStr *res = vm.makeVar<VarStr>(loc, "");
	while((read = getline(&lineptr, &len, file)) != -1) {
		String tmp = lineptr;
		while(!tmp.empty() && (tmp.back() == '\n' || tmp.back() == '\r')) {
			tmp.pop_back();
		}
		res->get() += tmp;
		free(lineptr);
		lineptr = NULL;
	}
	if(lineptr) free(lineptr);
	fseek(file, 0, SEEK_SET);
	return res;
}

Var *fileReadBlocks(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
		    const StringMap<AssnArgData> &assn_args)
{
	FILE *const file = as<VarFile>(args[0])->getFile();

	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for block begin location, found: ",
			vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for block end location, found: ",
			vm.getTypeName(args[2]));
		return nullptr;
	}

	const String &beg = as<VarStr>(args[1])->get();
	const String &end = as<VarStr>(args[2])->get();

	bool inside_block = false;
	char *lineptr	  = NULL;
	size_t len	  = 0;
	ssize_t read	  = 0;

	StringRef line;
	String block_content;
	VarVec *res = vm.makeVar<VarVec>(loc, 0, false);
	while((read = getline(&lineptr, &len, file)) != -1) {
		line = lineptr;
	begin_fetch:
		if(line.empty()) continue;
		if(!inside_block) {
			size_t pos = line.find(beg);
			if(pos == String::npos) continue;
			inside_block = true;
			if(pos + beg.size() > line.size()) continue;
			else line = line.substr(pos + beg.size());
			goto begin_fetch;
		}
		size_t pos = line.find(end);
		if(pos == String::npos) {
			block_content += line;
			continue;
		}
		block_content += line.substr(0, pos);
		if(pos + end.size() <= line.size()) line = line.substr(pos + end.size());
		else line = "";
		inside_block = false;
		res->push(vm.makeVarWithRef<VarStr>(loc, block_content));
		block_content.clear();
		goto begin_fetch;
	}

	if(lineptr) free(lineptr);
	fseek(file, 0, SEEK_SET);

	// this should actually never occur since block_content
	// is always pushed back when end point is found
	// if( !block_content.empty() ) {
	// 	res->push(vm.makeVar<VarStr>(loc, block_content));
	// }
	assert(block_content.empty());

	return res;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// File Descriptor Functions //////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: write tests

// equivalent to open(path, O_WRONLY | O_CREAT | O_TRUNC, mode)
Var *fdCreate(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	      const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for path, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarInt>()) {
		vm.fail(loc, "expected int argument for mode, found: ", vm.getTypeName(args[2]));
		return nullptr;
	}
	const String &path = as<VarStr>(args[1])->get();
	int res		   = creat(path.c_str(), as<VarInt>(args[2])->get());
	if(res < 0) {
		vm.fail(loc, "failed to create file: '", path, "', error: ", strerror(errno));
		return nullptr;
	}
	return vm.makeVar<VarInt>(loc, res);
}

Var *fdOpen(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarStr>()) {
		vm.fail(loc, "expected string argument for path, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarInt>()) {
		vm.fail(loc,
			"expected int argument for open flags, found: ", vm.getTypeName(args[2]));
		return nullptr;
	}
	const String &path = as<VarStr>(args[1])->get();
	int res		   = open(path.c_str(), as<VarInt>(args[2])->get());
	if(res < 0) {
		vm.fail(loc, "failed to open file: '", path, "', error: ", strerror(errno));
		return nullptr;
	}
	return vm.makeVar<VarInt>(loc, res);
}

Var *fdRead(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	    const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(
		loc, "expected int argument for file descriptor, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarBytebuffer>()) {
		vm.fail(loc, "expected bytebuffer containing data to write, found: ",
			vm.getTypeName(args[2]));
		return nullptr;
	}
	VarBytebuffer *bb = as<VarBytebuffer>(args[2]);
	errno		  = 0;
	ssize_t res	  = read(as<VarInt>(args[1])->get(), bb->getBuf(), bb->capacity());
	if(res < 0 || errno != 0) {
		vm.fail(loc, "failed to read from the file descriptor: '",
			std::to_string(as<VarInt>(args[1])->get()), "', error: ", strerror(errno));
		return nullptr;
	}
	bb->setLen(res);
	return vm.makeVar<VarInt>(loc, res);
}

Var *fdWrite(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(
		loc, "expected int argument for file descriptor, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	if(!args[2]->is<VarBytebuffer>() && !args[2]->is<VarStr>()) {
		vm.fail(loc, "expected bytebuffer/string containing data to write, found: ",
			vm.getTypeName(args[2]));
		return nullptr;
	}
	const void *data = nullptr;
	size_t count	 = 0;
	errno		 = 0;
	if(args[2]->is<VarBytebuffer>()) {
		VarBytebuffer *bb = as<VarBytebuffer>(args[2]);
		data		  = bb->getBuf();
		count		  = bb->capacity();
	} else if(args[2]->is<VarStr>()) {
		VarStr *s = as<VarStr>(args[2]);
		data	  = s->get().data();
		count	  = s->get().size();
	}
	ssize_t res = write(as<VarInt>(args[1])->get(), data, count);
	if(res < 0 || errno != 0) {
		vm.fail(loc, "failed to write to the file descriptor: '",
			std::to_string(as<VarInt>(args[1])->get()), "', error: ", strerror(errno));
		return nullptr;
	}
	return vm.makeVar<VarInt>(loc, res);
}

Var *fdClose(Interpreter &vm, const ModuleLoc *loc, Span<Var *> args,
	     const StringMap<AssnArgData> &assn_args)
{
	if(!args[1]->is<VarInt>()) {
		vm.fail(
		loc, "expected int argument for file descriptor, found: ", vm.getTypeName(args[1]));
		return nullptr;
	}
	int res = close(as<VarInt>(args[1])->get());
	if(res < 0) {
		vm.fail(loc, "failed to close the file descriptor: '",
			std::to_string(as<VarInt>(args[1])->get()), "', error: ", strerror(errno));
		return nullptr;
	}
	return vm.makeVar<VarInt>(loc, res);
}

INIT_MODULE(FS)
{
	VarModule *mod = vm.getCurrModule();

	vm.registerType<VarFile>(loc, "File");
	vm.registerType<VarFileIterator>(loc, "FileIterator");

	mod->addNativeFn("exists", fsExists, 1);
	mod->addNativeFn("fopenNative", fsOpen, 2);
	mod->addNativeFn("walkDirNative", fsWalkDir, 3);

	vm.addNativeTypeFn<VarFile>(loc, "reopenNative", fileReopen, 2);
	vm.addNativeTypeFn<VarFile>(loc, "lines", fileLines, 0);
	vm.addNativeTypeFn<VarFile>(loc, "seek", fileSeek, 2);
	vm.addNativeTypeFn<VarFile>(loc, "eachLine", fileEachLine, 0);
	vm.addNativeTypeFn<VarFile>(loc, "readAll", fileReadAll, 0);
	vm.addNativeTypeFn<VarFile>(loc, "readBlocks", fileReadBlocks, 2);

	vm.addNativeTypeFn<VarFileIterator>(loc, "next", fileIteratorNext, 0);

	// file descriptor
	mod->addNativeFn("fdOpenNative", fdOpen, 2);
	mod->addNativeFn("fdRead", fdRead, 2);
	mod->addNativeFn("fdWrite", fdWrite, 2);
	mod->addNativeFn("fdClose", fdClose, 1);

	// constants

	// stdin, stdout, stderr file descriptors
	mod->addNativeVar("stdin", vm.makeVar<VarInt>(loc, STDIN_FILENO));
	mod->addNativeVar("stdout", vm.makeVar<VarInt>(loc, STDOUT_FILENO));
	mod->addNativeVar("stderr", vm.makeVar<VarInt>(loc, STDERR_FILENO));

	// fs.walkdir()
	mod->addNativeVar("WALK_FILES", vm.makeVar<VarInt>(loc, WalkEntry::FILES));
	mod->addNativeVar("WALK_DIRS", vm.makeVar<VarInt>(loc, WalkEntry::DIRS));
	mod->addNativeVar("WALK_RECURSE", vm.makeVar<VarInt>(loc, WalkEntry::RECURSE));

	// <file>.seek()
	mod->addNativeVar("SEEK_SET", vm.makeVar<VarInt>(loc, SEEK_SET));
	mod->addNativeVar("SEEK_CUR", vm.makeVar<VarInt>(loc, SEEK_CUR));
	mod->addNativeVar("SEEK_END", vm.makeVar<VarInt>(loc, SEEK_END));

	// file descriptor flags
	mod->addNativeVar("O_RDONLY", vm.makeVar<VarInt>(loc, O_RDONLY));
	mod->addNativeVar("O_WRONLY", vm.makeVar<VarInt>(loc, O_WRONLY));
	mod->addNativeVar("O_RDWR", vm.makeVar<VarInt>(loc, O_RDWR));
	mod->addNativeVar("O_APPEND", vm.makeVar<VarInt>(loc, O_APPEND));
	mod->addNativeVar("O_CREAT", vm.makeVar<VarInt>(loc, O_CREAT));
#if defined(FER_OS_LINUX) || defined(FER_OS_APPLE)
	mod->addNativeVar("O_DSYNC", vm.makeVar<VarInt>(loc, O_DSYNC));
#endif
	mod->addNativeVar("O_EXCL", vm.makeVar<VarInt>(loc, O_EXCL));
#if !defined(FER_OS_WINDOWS)
	mod->addNativeVar("O_NOCTTY", vm.makeVar<VarInt>(loc, O_NOCTTY));
	mod->addNativeVar("O_NONBLOCK", vm.makeVar<VarInt>(loc, O_NONBLOCK));
#endif
#if defined(FER_OS_LINUX)
	mod->addNativeVar("O_RSYNC", vm.makeVar<VarInt>(loc, O_RSYNC));
#endif
#if !defined(FER_OS_WINDOWS)
	mod->addNativeVar("O_SYNC", vm.makeVar<VarInt>(loc, O_SYNC));
#endif
	mod->addNativeVar("O_TRUNC", vm.makeVar<VarInt>(loc, O_TRUNC));
	return true;
}

void getEntriesInternal(Interpreter &vm, const ModuleLoc *loc, const String &dirstr, VarVec *v,
			Regex regex, size_t flags)
{
	namespace fs = std::filesystem;
	String entry;
	for(const auto &ent : fs::directory_iterator(dirstr)) {
		if(ent.path() == "." || ent.path() == "..") continue;
		entry.clear();
		entry += ent.path().string();
		if((!(flags & WalkEntry::RECURSE) || !ent.is_directory()) &&
		   !std::regex_match(entry, regex))
		{
			continue;
		}
		if(ent.is_directory()) {
			if(flags & WalkEntry::RECURSE) {
				getEntriesInternal(vm, loc, entry, v, regex, flags);
			} else if(flags & WalkEntry::DIRS) {
				v->push(vm.makeVarWithRef<VarStr>(loc, entry));
			}
			continue;
		}
		if(flags & WalkEntry::FILES || flags & WalkEntry::RECURSE) {
			v->push(vm.makeVarWithRef<VarStr>(loc, entry));
		}
	}
}

} // namespace fer