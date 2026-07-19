#pragma once

// Defines all the common macros, types, etc. to be used by the codebase.
// Also contains all the standard library headers that are used.
// Should be a precompiled header.

#include <array>
#include <atomic>
#include <cassert>
#include <cstring>
#include <deque>
#include <filesystem>
#include <forward_list>
#include <fstream>
#include <future>
#include <initializer_list>
#include <iostream>
#include <mutex>
#include <regex>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#if defined(BUILD_DEBUG)
#define FER_BUILD_DEBUG
#else
#define FER_BUILD_RELEASE
#endif

#if defined(_WIN32) && defined(_MSC_VER)
#define FER_OS_WINDOWS
#if defined(_WIN64)
#define FER_OS_WINDOWS64
#endif
#elif defined(__linux__)
#define FER_OS_LINUX
#elif defined(__ANDROID__)
#define FER_OS_ANDROID
#elif defined(__FreeBSD__)
#define FER_OS_BSD
#define FER_OS_FREEBSD
#elif defined(__NetBSD__)
#define FER_OS_BSD
#define FER_OS_NETBSD
#elif defined(__OpenBSD__)
#define FER_OS_BSD
#define FER_OS_OPENBSD
#elif defined(__bsdi__)
#define FER_OS_BSD
#define FER_OS_BSDI
#elif defined(__DragonFly__)
#define FER_OS_BSD
#define FER_OS_DRAGONFLYBSD
#elif defined(__APPLE__)
#define FER_OS_APPLE
#endif

#if defined(FER_OS_WINDOWS)
#define FER_API_EXPORT __declspec(dllexport)
#define FER_API_IMPORT __declspec(dllimport)
#if defined(EXPORT_FOR_DLL)
#define FER_API FER_API_EXPORT
#else
#define FER_API FER_API_IMPORT
#endif
#else
#define FER_API_EXPORT
#define FER_API_IMPORT
#define FER_API
#endif

#if defined(FER_OS_WINDOWS)
#if defined(FER_OS_WINDOWS64)
using ssize_t = int64_t;
#else
using ssize_t = int32_t;
#endif
// Windows doesn't have unistd.h which defines these.
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif

namespace fer
{

namespace fs = std::filesystem;

using Path           = fs::path;
using Mutex          = std::mutex;
using Regex          = std::regex;
using String         = std::string;
using Thread         = std::thread;
using JThread        = std::jthread;
using IStream        = std::istream;
using OStream        = std::ostream;
using FStream        = std::fstream;
using Nullptr        = std::nullptr_t;
using IOStream       = std::iostream;
using IFStream       = std::ifstream;
using OFStream       = std::ofstream;
using StringRef      = std::string_view;
using RecursiveMutex = std::recursive_mutex;

#if defined(FER_OS_WINDOWS)
using WString    = std::wstring;
using WStringRef = std::wstring_view;
#endif

struct StringHash
{
    using HashType       = std::hash<StringRef>;
    using is_transparent = void;

    size_t operator()(const char *str) const { return HashType{}(str); }
    size_t operator()(StringRef str) const { return HashType{}(str); }
    size_t operator()(const String &str) const { return HashType{}(str); }
};

template<typename T> using Set             = std::unordered_set<T>;
template<typename T> using Span            = std::span<T>;
template<typename T> using Deque           = std::deque<T>;
template<typename T> using Atomic          = std::atomic<T>;
template<typename T> using Vector          = std::vector<T>;
template<typename T> using UniList         = std::forward_list<T>; // singly linked list
template<typename T> using InitList        = std::initializer_list<T>;
template<typename T> using LockGuard       = std::lock_guard<T>;
template<typename... Ts> using Variant     = std::variant<Ts...>;
template<typename T> using SharedFuture    = std::shared_future<T>;
template<typename Fn> using PackagedTask   = std::packaged_task<Fn>;
template<typename T, size_t N> using Array = std::array<T, N>;
template<typename K, typename V> using Map = std::unordered_map<K, V>;
template<typename V> using StringMap = std::unordered_map<String, V, StringHash, std::equal_to<>>;

constexpr size_t MAX_PATH_CHARS = 4096;
constexpr size_t MAX_ENV_CHARS  = 4096;

#if defined(FER_OS_WINDOWS)
// Windows' max recurse count seems to be ~200. So 100 should do.
constexpr size_t DEFAULT_MAX_RECURSE_COUNT = 100;
#else
constexpr size_t DEFAULT_MAX_RECURSE_COUNT = 400;
#endif

constexpr char const *PRELUDE_MODULE = "prelude/prelude";

constexpr char const *MODULE_EXTENSION        = ".fer";
constexpr char const *MODULE_EXTENSION_NO_DOT = "fer";

#include "Config.inl"

} // namespace fer