#pragma once

#include "Core.hpp"

namespace fer
{
namespace env
{

String get(const char *key);
String getProcPath();
String getExeFromPath(const char *exe);

inline int set(const char *key, const char *val, bool overwrite)
{
	return setenv(key, val, overwrite);
}

} // namespace env
} // namespace fer