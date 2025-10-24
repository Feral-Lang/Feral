#pragma once

#include "Core.hpp"

namespace fer::tio
{

void taba(bool show);
void tabr(size_t num = 1);
void print(bool hasNext, InitList<StringRef> data);
void printf(InitList<StringRef> data);

} // namespace fer::tio