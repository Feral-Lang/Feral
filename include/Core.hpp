#pragma once

#include <array>
#include <cassert>
#include <cstring>
#include <deque>
#include <forward_list>
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

namespace fer
{

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

// the primitives have lower case name
using u8	= unsigned char;
using uiptr	= std::uintptr_t;
using Mutex	= std::mutex;
using Regex	= std::regex;
using String	= std::string;
using Thread	= std::thread;
using IStream	= std::istream;
using Nullptr	= std::nullptr_t;
using OStream	= std::ostream;
using IOStream	= std::iostream;
using StringRef = std::string_view;

struct StringHash
{
	using HashType	     = std::hash<StringRef>;
	using is_transparent = void;

	size_t operator()(const char *str) const { return HashType{}(str); }
	size_t operator()(StringRef str) const { return HashType{}(str); }
	size_t operator()(const String &str) const { return HashType{}(str); }
};

template<typename T> using Set		   = std::unordered_set<T>;
template<typename T> using List		   = std::forward_list<T>; // singly linked list
template<typename T> using Span		   = std::span<T>;
template<typename T> using Deque	   = std::deque<T>;
template<typename T> using Vector	   = std::vector<T>;
template<typename T> using InitList	   = std::initializer_list<T>;
template<typename T> using LockGuard	   = std::lock_guard<T>;
template<typename... Ts> using Variant	   = std::variant<Ts...>;
template<typename T> using SharedFuture	   = std::shared_future<T>;
template<typename Fn> using PackagedTask   = std::packaged_task<Fn>;
template<typename T, size_t N> using Array = std::array<T, N>;
template<typename K, typename V> using Map = std::unordered_map<K, V>;
template<typename V> using StringMap = std::unordered_map<String, V, StringHash, std::equal_to<>>;

} // namespace fer