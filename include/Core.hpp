#pragma once

// Should be a precompiled header.

#include "LibCore.hpp"

#if defined(CORE_OS_WINDOWS)
#if defined(EXPORT_FOR_DLL)
#define FER_API __declspec(dllexport)
#else
#define FER_API __declspec(dllimport)
#endif
#else
#define FER_API
#endif

namespace fer
{

using namespace core;

#include "Config.inl"

} // namespace fer