#pragma once

#include "Core.hpp"

#define MAX_PATH_CHARS 4096

#if defined(FER_OS_WINDOWS)
#define MAX_ENV_CHARS MAX_PATH_CHARS
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

ssize_t getdelim(char **buf, size_t *bufsiz, int delimiter, FILE *fp);
ssize_t getline(char **buf, size_t *bufsiz, FILE *fp);
#endif

namespace fer
{
namespace fs
{

extern int total_lines;
inline void setTotalLines(int lines) { total_lines = lines; }
inline int getTotalLines() { return total_lines; }

StringRef parentDir(StringRef path);

bool exists(StringRef loc);
bool read(const char *file, String &data, bool ignoreErrorOut = false);
String absPath(const char *loc);
bool setCWD(const char *path);
String getCWD();
StringRef home();

int copy(StringRef src, StringRef dest, std::error_code &ec);
int mkdir(StringRef dir, std::error_code &ec);
int mklink(StringRef src, StringRef dest, std::error_code &ec);
int rename(StringRef from, StringRef to, std::error_code &ec);
int remove(StringRef path, std::error_code &ec);

} // namespace fs
} // namespace fer