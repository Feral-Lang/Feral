#include "FS.hpp"

#include <cstdlib>
#include <cstring>
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

bool exists(const String &loc) { return access(loc.c_str(), F_OK) != -1; }

bool read(const String &file, String &data)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	fp = fopen(file.c_str(), "r");
	if(fp == NULL) {
		fprintf(stderr, "Error: failed to open source file: %s\n", file.c_str());
		return false;
	}

	while((read = getline(&line, &len, fp)) != -1) {
		data += line;
		if(read > 1 || (read == 1 && line[0] == '\n')) ++total_lines;
	}

	fclose(fp);
	if(line) free(line);

	if(data.empty()) {
		fprintf(stderr, "Error: encountered empty file: %s\n", file.c_str());
		return false;
	}

	return true;
}

String absPath(const String &loc)
{
	static char abs[MAX_PATH_CHARS];
	static char abs_tmp[MAX_PATH_CHARS];
	realpath(loc.c_str(), abs);
	return abs;
}

String getCWD()
{
	static char cwd[MAX_PATH_CHARS];
	if(getcwd(cwd, sizeof(cwd)) != NULL) {
		return cwd;
	}
	return "";
}

bool setCWD(const String &path) { return chdir(path.c_str()) != 0; }

String parentDir(const String &path) { return path.substr(0, path.find_last_of("/\\")); }

String home()
{
	static String _home = env::get("HOME");
	return _home;
}

} // namespace fs
} // namespace fer