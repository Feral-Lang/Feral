#pragma once

#include "InterpreterState.hpp"
#include "InterpreterThread.hpp"

namespace fer
{

class Interpreter
{
	InterpreterState globalState;
	// Used to store InterpreterThread memory after one is free'd
	UniList<void *> freeMem;

	bool loadPrelude();

public:
	Interpreter(ArgParser &argparser, ParseSourceFn parseSourceFn);
	~Interpreter();

	// Returns VarInt if useThisThread is true
	// Returns VarThread otherwise
	int runFile(ModuleLoc loc, const char *file);

	InterpreterThread *createThread();
	void destroyThread(InterpreterThread *vm);

	inline InterpreterState &getGlobalState() { return globalState; }
};

} // namespace fer