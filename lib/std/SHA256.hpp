#pragma once

#include "VM/VarTypes.hpp"

namespace fer
{

class VarSHA256Ctx : public Var
{
    uint32_t H[8];
    unsigned char pending[64];
    size_t pendingLen;
    uint64_t msgLen; // total bytes fed so far

public:
    VarSHA256Ctx(ModuleLoc loc);

    void reset();
    void update(const unsigned char *data, size_t len);
    // Non-mutating: returns the hex digest without modifying the context.
    String finalize() const;
};

} // namespace fer
