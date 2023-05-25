#pragma once

#include <unistd.h>

#include "Core.hpp"

#define MAX_PATH_CHARS 4096

namespace fer
{
namespace fs
{

extern int total_lines;
inline void setTotalLines(int lines) { total_lines = lines; }
inline int getTotalLines() { return total_lines; }

inline bool exists(const char *loc) { return access(loc, F_OK) != -1; }
inline bool setCWD(const char *path) { return chdir(path) != 0; }
inline StringRef parentDir(StringRef path) { return path.substr(0, path.find_last_of("/\\")); }

bool read(const char *file, String &data);
String absPath(const char *loc);
String getCWD();
String home();

inline int rename(const char *from, const char *to) { return std::rename(from, to); }

} // namespace fs
} // namespace fer