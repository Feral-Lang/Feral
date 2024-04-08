#pragma once

#include "Core.hpp"

namespace fer
{
namespace env
{

String get(const char *key);
String getProcPath();
String getExeFromPath(const char *exe);

bool set(const char *key, const char *val, bool overwrite);

} // namespace env
} // namespace fer