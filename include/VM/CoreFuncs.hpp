#include "Interpreter.hpp"

namespace fer
{

void setupCoreFuncs(InterpreterManager &im, ModuleLoc loc);

// This one is setup in Interpreter.cpp since it is required to be appended to moduleFinders before
// any module (feral/native) can be loaded.
Var *basicModuleFinder(Interpreter &vm, ModuleLoc loc, Span<Var *> args,
		       const StringMap<AssnArgData> &assn_args);

} // namespace fer