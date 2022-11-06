#pragma once

#include "Core.hpp"

namespace fer
{
namespace env
{

String get(const String &key);
String getProcPath();
String getExeFromPath(const String &exe);

} // namespace env
} // namespace fer