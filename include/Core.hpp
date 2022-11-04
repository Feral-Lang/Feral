#pragma once

#include <forward_list>
#include <initializer_list>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fer
{

using String				   = std::string;
using StringRef				   = std::string_view;
using IStream				   = std::istream;
using OStream				   = std::ostream;
using IOStream				   = std::iostream;
template<typename T> using Set		   = std::unordered_set<T>;
template<typename T> using List		   = std::forward_list<T>; // singly linked list
template<typename T> using Vector	   = std::vector<T>;
template<typename T> using InitList	   = std::initializer_list<T>;
template<typename K, typename V> using Map = std::unordered_map<K, V>;

} // namespace fer