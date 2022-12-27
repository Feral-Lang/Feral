#include "VM/Interpreter.hpp"

#include <cstring>

#include "Env.hpp"
#include "Error.hpp"
#include "FS.hpp"
#include "Utils.hpp"
#include "VM/DynLib.hpp"

namespace fer
{

size_t MAX_RECURSE_COUNT = DEFAULT_MAX_RECURSE_COUNT;

Interpreter::Interpreter(Context &c, ArgParser &argparser)
	: c(c), argparser(argparser), tru(makeVar<VarBool>(nullptr, true)),
	  fals(makeVar<VarBool>(nullptr, false)), nil(makeVar<VarNil>(nullptr)), exitcode(0),
	  recurse_count(0), exit_called(false), recurse_count_exceeded(false)
{
	initTypeNames();

	selfbin	 = c.moveStr(env::getProcPath());
	selfbase = fs::parentDir(fs::parentDir(selfbin));
	char base[MAX_PATH_CHARS];
	strncpy(base, selfbase.data(), selfbase.size());
	base[selfbase.size()] = '\0';
	selfbase	      = c.moveStr(fs::absPath(base));

	Span<StringRef> _cmdargs = argparser.getCodeExecArgs();
	cmdargs			 = makeVar<VarVec>(nullptr, _cmdargs.size(), false);
	for(auto &a : _cmdargs) {
		cmdargs->get().push_back(makeVar<VarStr>(nullptr, a));
	}

	includelocs.push_back(c.strFrom({selfbase, "/include/feral"}));
	dlllocs.push_back(c.strFrom({selfbase, "/lib/feral"}));

	String feral_paths = env::get("FERAL_PATHS");
	for(auto &path : stringDelim(feral_paths, ";")) {
		includelocs.push_back(c.strFrom({path, "/include/feral"}));
		dlllocs.push_back(c.strFrom({path, "/lib/feral"}));
	}
}

// TODO:
Interpreter::~Interpreter()
{
	decref(nil);
	decref(fals);
	decref(tru);
	decref(cmdargs);
	for(auto &typefn : typefns) {
		delete typefn.second;
	}
	for(auto &deinitfn : dlldeinitfns) {
		deinitfn.second();
	}
	for(auto &mod : allmodules) decref(mod.second);
}

void Interpreter::pushModule(const ModuleLoc *loc, Module *mod)
{
	auto mloc	= allmodules.find(mod->getPath());
	VarModule *mvar = nullptr;
	if(mloc == allmodules.end()) {
		mvar = makeVar<VarModule>(loc, mod);
		allmodules.insert({mod->getPath(), mvar});
	} else {
		mvar = mloc->second;
	}
	incref(mvar);
	modulestack.push_back(mvar);
}
void Interpreter::pushModule(StringRef path)
{
	auto mloc = allmodules.find(path);
	incref(mloc->second);
	modulestack.push_back(mloc->second);
}
void Interpreter::popModule()
{
	decref(modulestack.back());
	modulestack.pop_back();
}

bool Interpreter::moduleExists(Span<StringRef> locs, String &mod)
{
	size_t count = mod.size();
	static char testpath[MAX_PATH_CHARS];
	if(mod.front() != '~' && mod.front() != '/' && mod.front() != '.') {
		for(auto loc : locs) {
			strncpy(testpath, loc.data(), loc.size());
			testpath[loc.size()] = '\0';
			strcat(testpath, "/");
			strcat(testpath, mod.c_str());
		}
	} else {
		if(mod.front() == '~') {
			mod.erase(mod.begin());
			static String home = env::get("HOME");
			mod.insert(mod.begin(), home.begin(), home.end());
		} else if(mod.front() == '.') {
			assert(modulestack.size() > 0 &&
			       "dot based module search cannot be done on empty modulestack");
			StringRef dir = modulestack.back()->getMod()->getPath();
			mod.erase(mod.begin());
			mod.insert(mod.begin(), dir.begin(), dir.end());
		}
		strcpy(testpath, mod.c_str());
	}
	strcat(testpath, MODULE_EXTENSION);
	if(fs::exists(testpath)) {
		mod = fs::absPath(testpath);
		return true;
	}
	return false;
}

bool Interpreter::loadNativeModule(const ModuleLoc *loc, StringRef modstr)
{
	StringRef mod	 = modstr.substr(modstr.find_last_of('/') + 1);
	StringRef moddir = fs::parentDir(modstr);
	String modfilestr(modstr);
	modfilestr += getNativeModuleExtension();
	modfilestr.insert(modfilestr.find_last_of('/') + 1, "libferal");
	if(!moduleExists(dlllocs, modfilestr)) {
		fail(loc, {"module: ", mod, " not found"});
		return false;
	}

	StringRef modfile = c.moveStr(std::move(modfilestr));

	DynLib &dlibs = DynLib::getInstance();
	if(dlibs.exists(modfile)) return true;

	if(!dlibs.load(modfile)) {
		fail(loc, {"unable to load module file: ", modfile});
		return false;
	}

	String tmp = "init_";
	tmp += mod;
	ModInitFn initfn = (ModInitFn)dlibs.get(modfile, tmp.c_str());
	if(initfn == nullptr) {
		fail(loc, {"unable to load init function from module file: ", modfile});
		dlibs.unload(modfile);
		return false;
	}
	if(!initfn(*this, loc)) {
		fail(loc, {"init function in module: ", modfile, " failed to execute"});
		dlibs.unload(modfile);
		return false;
	}
	// set deinit function if available
	tmp = "deinit_";
	tmp += mod;
	ModDeinitFn deinitfn = (ModDeinitFn)dlibs.get(modfile, tmp.c_str());
	if(deinitfn) dlldeinitfns[modfile] = deinitfn;
	return true;
}

void Interpreter::addGlobal(StringRef name, Var *val, bool iref)
{
	if(globals.exists(name)) return;
	globals.add(name, val, iref);
}

void Interpreter::addTypeFn(uiptr _typeid, StringRef name, Var *fn, bool iref)
{
	auto loc    = typefns.find(_typeid);
	VarFrame *f = nullptr;
	if(loc == typefns.end()) {
		typefns[_typeid] = f = new VarFrame;
	} else {
		f = loc->second;
	}
	if(f->exists(name)) {
		err::out(nullptr, {"type function: ", name, " already exists"});
		assert(false);
	}
	f->add(name, fn, iref);
}
Var *Interpreter::getTypeFn(Var *var, StringRef name)
{
	auto loc = typefns.find(var->getTypeFnID());
	Var *res = nullptr;
	if(loc != typefns.end()) {
		res = loc->second->get(name);
		if(res) return res;
	} else if(var->isAttrBased()) {
		loc = typefns.find(var->getType());
		if(loc != typefns.end()) {
			res = loc->second->get(name);
			if(res) return res;
		}
	}
	return typefns[typeID<VarAll>()]->get(name);
}

StringRef Interpreter::getTypeName(uiptr _typeid)
{
	auto loc = typenames.find(_typeid);
	if(loc == typenames.end()) return c.strFrom({"typeID<", std::to_string(_typeid), ">"});
	return loc->second;
}

Var *Interpreter::getConst(const ModuleLoc *loc, Data &d, DataType dataty)
{
	switch(dataty) {
	case DataType::BOOL: return d.b ? tru : fals;
	case DataType::NIL: return nil;
	case DataType::CHR: return makeVar<VarChar>(loc, d.c);
	case DataType::INT: return makeVar<VarInt>(loc, d.i);
	case DataType::FLT: return makeVar<VarFlt>(loc, d.d);
	case DataType::STR: return makeVar<VarStrRef>(loc, d.s);
	default: err::out(*loc, {"internal error: invalid data type encountered"});
	}
	return nullptr;
}

void Interpreter::initTypeNames()
{
	registerType<VarAll>("all");

	registerType<VarNil>("nil");
	registerType<VarBool>("bool");
	registerType<VarInt>("int");
	registerType<VarFlt>("flt");
	registerType<VarStr>("str");
	registerType<VarStrRef>("strref");
	registerType<VarVec>("vec");
	registerType<VarMap>("map");
	registerType<VarFn>("func");
	registerType<VarModule>("module");
	registerType<VarTypeID>("typeid");
}

void Interpreter::fail(const ModuleLoc *loc, InitList<StringRef> err)
{
	// if there is no block in the failstack, simply show the error
	if(failstack.empty() || exit_called) {
		err::out(*loc, err);
	} else {
		failstack.push(makeVar<VarStr>(loc, err), false);
	}
}

} // namespace fer