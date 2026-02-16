#pragma once

#include "VarTypes.hpp"

namespace fer
{

typedef bool (*ParseSourceFn)(VirtualMachine &vm, Bytecode &bc, ModuleId moduleId, StringRef path,
                              StringRef data, bool exprOnly);

class GlobalState
{
    MemoryManager mem;
    ManagedList managedAllocator;
    args::ArgParser &argparser;
    ParseSourceFn parseSourceFn;
    RecursiveMutex mutex;
    Atomic<size_t> vmCount;
    // Names of types (optional)
    Map<size_t, String> typenames;
    // Prelude must be imported before any program is executed
    String prelude;
    // Default error handler
    VarFn *basicErrHandler;
    // Global vars/objects that are required
    VarMap *globals;
    // Functions for all C++ types
    Map<size_t, VarMap *> typefns;
    // Default dirs to search for modules. Used by basic{Import,Module}Finder()
    VarVec *moduleDirs;
    // Functions (VarVec<VarFn>) to resolve module locations. If one fails, next one is
    // attempted.
    // Signature is: fn(moduleToResolve: str, isImport: bool): nil/str
    VarVec *moduleFinders;
    // Various paths used by Feral and are provided as <prelude>.<pathVar>.
    VarStr *binaryPath;        // where feral exists
    VarStr *installPath;       // <binaryPath>/..
    VarStr *tempPath;          // <binaryPath>/../tmp
    VarStr *libPath;           // <binaryPath>/../lib/feral
    VarStr *globalModulesPath; // <libPath>/.modulePaths
    // Args provided to feral command line
    VarVec *cmdargs;
    VarBool *tru;
    VarBool *fals;
    VarNil *nil;
    // Imports
    Map<ModuleId, VarModule *> modules;
    // This is the one that's used for checking, and it can be modified by Feral program
    size_t recurseMax;
    Atomic<bool> stopExec;

    friend class VirtualMachine;

    bool init(VirtualMachine &vm);
    bool deinit(VirtualMachine &vm);

public:
    GlobalState(args::ArgParser &argparser, ParseSourceFn parseSourceFn);
    ~GlobalState();
};

} // namespace fer