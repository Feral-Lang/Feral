#pragma once

#include "Core.hpp"

#define MAX_PATH_CHARS 4096

#if defined(OS_WINDOWS)
#define MAX_ENV_CHARS MAX_PATH_CHARS
// #define PATH_DELIM "\\"
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

ssize_t getdelim(char **buf, size_t *bufsiz, int delimiter, FILE *fp);
ssize_t getline(char **buf, size_t *bufsiz, FILE *fp);
#else
// #define PATH_DELIM "/"
#endif

namespace fer
{
namespace fs
{

extern int total_lines;
inline void setTotalLines(int lines) { total_lines = lines; }
inline int getTotalLines() { return total_lines; }

inline StringRef parentDir(StringRef path) { return path.substr(0, path.find_last_of("/\\")); }

bool exists(const char *loc);
bool read(const char *file, String &data);
String absPath(const char *loc);
bool setCWD(const char *path);
String getCWD();
String home();

inline int rename(const char *from, const char *to) { return std::rename(from, to); }

} // namespace fs
} // namespace fer