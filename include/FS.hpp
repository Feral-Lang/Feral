#pragma once

#include "Core.hpp"

#define MAX_PATH_CHARS 4096

namespace fer
{
namespace fs
{

int getTotalLines();
bool exists(const char *loc);
bool exists(const String &loc);
bool read(const String &file, String &data);
String absPath(const char *loc);
String absPath(const String &loc);
String getCWD();
bool setCWD(const String &path);
StringRef parentDir(StringRef path);
String home();

} // namespace fs
} // namespace fer