#pragma once

#include "Core.hpp"

namespace fer
{
namespace tio
{

void taba(bool show);
void tabr(size_t num = 1);
void print(bool has_next, InitList<StringRef> data);
void printf(InitList<StringRef> data);

} // namespace tio
} // namespace fer