#pragma once

#include "VarTypes.hpp"

namespace fer
{

class FailStack
{
    Vector<VarFailure *> stack;
    MemoryManager &mem;

    void failStr(ModuleLoc loc, size_t recurseCount, String &&msg);

public:
    FailStack(MemoryManager &mem);
    ~FailStack();

    void pushHandler(VarFn *handler, size_t popLoc, size_t recurseCount, bool irefHandler);
    void popHandler();

    Var *handle(VirtualMachine &vm, ModuleLoc loc, size_t &popLoc);

    template<typename... Args> void fail(ModuleLoc loc, size_t recurseCount, Args &&...args)
    {
        failStr(loc, recurseCount, utils::toString(std::forward<Args>(args)...));
    }

    inline size_t getLastRecurseCount()
    {
        return stack.empty() ? 0 : stack.back()->getRecurseCount();
    }
    inline size_t size() const { return stack.size(); }
};

} // namespace fer