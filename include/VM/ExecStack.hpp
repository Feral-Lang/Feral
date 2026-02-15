#pragma once

#include "VarTypes.hpp"

namespace fer
{

class ExecStack : public IAllocated
{
    Vector<Var *> stack;
    MemoryManager &mem;

public:
    ExecStack(MemoryManager &mem);
    ~ExecStack();

    void push(Var *val, bool iref = true);
    Var *pop(bool dref = true);

    inline Var *back() { return stack.back(); }
    inline Vector<Var *> &get() { return stack; }

    inline size_t size() const { return stack.size(); }
    inline bool empty() const { return stack.empty(); }
    inline void clear() { stack.clear(); }

    String dump(VirtualMachine *vm);
};

} // namespace fer