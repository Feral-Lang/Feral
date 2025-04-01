#include "VM/Interpreter.hpp"

#include "FS.hpp"
#include "VM/CoreFuncs.hpp"

namespace fer
{

Interpreter::Interpreter(ArgParser &argparser, ParseSourceFn parseSourceFn)
	: globalState(argparser, parseSourceFn)
{
	if(!loadPrelude()) throw "Failed to load prelude";
}
Interpreter::~Interpreter()
{
	for(auto &item : freeMem) {
		globalState.getMemoryManager().free(item);
	}
}

bool Interpreter::loadPrelude()
{
	VarFn *bmfFn = globalState.genNativeFn({}, "basicModuleFinder", basicModuleFinder, 2);
	globalState.moduleFinders->push(bmfFn);
	// loadlib must be setup here because it is needed to load even the core module from
	// <prelude>.
	setupCoreFuncs(globalState, {});
	if(!globalState.findImportModuleIn(globalState.moduleDirs, globalState.prelude,
					   fs::getCWD()))
	{
		err.fail({}, "Failed to find prelude: ", globalState.prelude);
		return 1;
	}
	int res = runFile({}, globalState.prelude.c_str());
	if(res != 0) {
		err.fail({}, "Failed to import prelude: ", globalState.prelude);
		return false;
	}
	// set the prelude/feral global variable
	globalState.addGlobal("feral", globalState.getModule(globalState.prelude));
	return true;
}

int Interpreter::runFile(ModuleLoc loc, const char *file)
{
	InterpreterThread *vm = createThread();
	int res		      = vm->compileAndRun(loc, file);
	destroyThread(vm);
	return res;
}

InterpreterThread *Interpreter::createThread()
{
	MemoryManager &mem    = globalState.getMemoryManager();
	InterpreterThread *vm = nullptr;
	if(!freeMem.empty()) {
		vm = new(freeMem.front()) InterpreterThread(*this);
		freeMem.pop_front();
	}
	if(!vm) vm = mem.alloc<InterpreterThread>(*this);
	return vm;
}

void Interpreter::destroyThread(InterpreterThread *vm)
{
	vm->~InterpreterThread();
	freeMem.push_front(vm);
}

} // namespace fer