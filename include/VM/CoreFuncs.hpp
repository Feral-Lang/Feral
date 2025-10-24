#include "Interpreter.hpp"

namespace fer
{

void setupCoreFuncs(Interpreter &ip, ModuleLoc loc);

// This one is setup in Interpreter.cpp since it is required to be appended to moduleFinders before
// any module (feral/native) can be loaded.
Var *basicModuleFinder(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                       const StringMap<AssnArgData> &assn_args);

// This is the default error handler which simply shows the error output using std::cerr
Var *basicErrorHandler(VirtualMachine &vm, ModuleLoc loc, Span<Var *> args,
                       const StringMap<AssnArgData> &assn_args);
} // namespace fer