#pragma once

#include "Core.hpp"

namespace fer::env
{

String FER_API get(const char *key);
Path FER_API getHome();
Path FER_API getProcPath();

bool FER_API set(const char *key, const char *val, bool overwrite);

int FER_API exec(const char *cmd);

} // namespace fer::env