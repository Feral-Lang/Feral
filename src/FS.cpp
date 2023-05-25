#include "FS.hpp"

#include <cstdlib>
#include <string>
#include <vector>

#include "Env.hpp"

namespace fer
{
namespace fs
{

int total_lines = 0;

bool read(const char *file, String &data)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	fp = fopen(file, "r");
	if(fp == NULL) {
		std::cerr << "Error: failed to open source file: " << file << "\n";
		return false;
	}

	while((read = getline(&line, &len, fp)) != -1) data += line;

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

String getCWD()
{
	static char cwd[MAX_PATH_CHARS];
	if(getcwd(cwd, sizeof(cwd)) != NULL) {
		return cwd;
	}
	return "";
}

String home()
{
	static String _home = env::get("HOME");
	return _home;
}

} // namespace fs
} // namespace fer