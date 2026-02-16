#include "VarTypes.hpp"

namespace fer
{

FERAL_FUNC_DECL(loadFile, 1, false,
                "  fn(file) -> Module\n"
                "Imports a feral script `file` and returns the imported Module.");

FERAL_FUNC_DECL(loadLibrary, 1, false,
                "  fn(file) -> Nil\n"
                "Loads a native (C/C++) object `file`.");

// This one is setup in VM.cpp since it is required to be appended to moduleFinders before
// any module (feral/native) can be loaded.
FERAL_FUNC_DECL(
    basicModuleFinder, 2, false,
    "  fn(name, isImport) -> Str\n"
    "Finds and returns the full path of a module using its `name` and knowing if it's `isImport`.\n"
    "This is the basic implementation used by Feral. Custom ones can be added to "
    "`feral.moduleFinders` vector.");

// This is the default error handler which simply shows the error output using std::cerr
FERAL_FUNC_DECL(
    basicErrorHandler, 1, false,
    "  fn(failure) -> RaisedError\n"
    "This is the default error handler which simply shows the error on std::cerr output stream.");

} // namespace fer