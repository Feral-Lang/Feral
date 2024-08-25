#pragma once

#include "Core.hpp"

namespace fer
{

#define MAX_PATH_CHARS 4096

#if defined(FER_OS_WINDOWS)
#define MAX_ENV_CHARS MAX_PATH_CHARS
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

ssize_t getdelim(char **buf, size_t *bufsiz, int delimiter, FILE *fp);
ssize_t getline(char **buf, size_t *bufsiz, FILE *fp);
#endif

namespace fs
{

// Not actually externed (at least on Windows), which works because
// setTotalLines and getTotalLines are to be used and not the variable itself.
extern int totalLines;
inline void setTotalLines(int lines) { totalLines = lines; }
inline int getTotalLines() { return totalLines; }

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