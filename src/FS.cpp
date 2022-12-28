#include "FS.hpp"

#include <cstdlib>
#include <string>
#include <unistd.h>
#include <vector>

#include "Env.hpp"

namespace fer
{
namespace fs
{

static int total_lines = 0;

int getTotalLines() { return total_lines; }

bool exists(const char *loc) { return access(loc, F_OK) != -1; }
bool exists(const String &loc) { return access(loc.c_str(), F_OK) != -1; }

bool read(const String &file, String &data)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	fp = fopen(file.c_str(), "r");
	if(fp == NULL) {
		std::cerr << "Error: failed to open source file: " << file << "\n";
		return false;
	}

	while((read = getline(&line, &len, fp)) != -1) {
		data += line;
		if(read > 1 || (read == 1 && line[0] == '\n')) ++total_lines;
	}

	fclose(fp);
	if(line) free(line);

	if(data.empty()) {
		std::cerr << "Error: encountered empty file: " << file << "\n";
		return false;
	}

	return true;
}

String absPath(const char *loc)
{
	static char abs[MAX_PATH_CHARS];
	realpath(loc, abs);
	return abs;
}

String absPath(const String &loc) { return absPath(loc.c_str()); }

String getCWD()
{
	static char cwd[MAX_PATH_CHARS];
	if(getcwd(cwd, sizeof(cwd)) != NULL) {
		return cwd;
	}
	return "";
}

bool setCWD(const String &path) { return chdir(path.c_str()) != 0; }

StringRef parentDir(StringRef path) { return path.substr(0, path.find_last_of("/\\")); }

String home()
{
	static String _home = env::get("HOME");
	return _home;
}

} // namespace fs
} // namespace fer