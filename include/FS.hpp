#pragma once

#include "Core.hpp"

#define MAX_PATH_CHARS 4096

namespace fer
{
namespace fs
{

int getTotalLines();
bool exists(const String &loc);
bool read(const String &file, String &data);
String absPath(const String &loc);
String getCWD();
bool setCWD(const String &path);
String parentDir(const String &path);
String home();

} // namespace fs
} // namespace fer