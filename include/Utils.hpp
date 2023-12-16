#pragma once

#include "Core.hpp"

namespace fer
{

// RAII class to manage a pointer
template<typename T> class Pointer
{
	T *data;

public:
	Pointer(T *dat) : data(dat) {}
	~Pointer()
	{
		if(data) delete data;
	}
	void set(T *dat) { data = dat; }
	void unset() { data = nullptr; }
};

inline bool startsWith(StringRef src, StringRef term) { return src.rfind(term, 0) == 0; }

// Also trims the spaces for each split
Vector<StringRef> stringDelim(StringRef str, StringRef delim);

// Convert special characters in string (\n, \t, ...) to raw (\\n, \\t, ...)
// and vice versa
String toRawString(StringRef data);
String fromRawString(StringRef data);

String vecToStr(Span<StringRef> items);
String vecToStr(Span<String> items);

inline void appendToString(String &dest) {}

template<typename... Args> void appendToString(String &dest, StringRef data, Args... args);
template<typename... Args> void appendToString(String &dest, char data, Args... args);
template<typename... Args> void appendToString(String &dest, u8 data, Args... args);
template<typename... Args> void appendToString(String &dest, int data, Args... args);
template<typename... Args> void appendToString(String &dest, size_t data, Args... args);
template<typename... Args> void appendToString(String &dest, int64_t data, Args... args);
template<typename... Args> void appendToString(String &dest, float data, Args... args);
template<typename... Args> void appendToString(String &dest, double data, Args... args);

template<typename... Args> void appendToString(String &dest, StringRef data, Args... args)
{
	dest += data;
	appendToString(dest, args...);
}
template<typename... Args> void appendToString(String &dest, char data, Args... args)
{
	dest += data;
	appendToString(dest, args...);
}
template<typename... Args> void appendToString(String &dest, u8 data, Args... args)
{
	dest += std::to_string(data);
	appendToString(dest, args...);
}
template<typename... Args> void appendToString(String &dest, int data, Args... args)
{
	dest += std::to_string(data);
	appendToString(dest, args...);
}
template<typename... Args> void appendToString(String &dest, size_t data, Args... args)
{
	dest += std::to_string(data);
	appendToString(dest, args...);
}
template<typename... Args> void appendToString(String &dest, int64_t data, Args... args)
{
	dest += std::to_string(data);
	appendToString(dest, args...);
}
template<typename... Args> void appendToString(String &dest, float data, Args... args)
{
	dest += std::to_string(data);
	appendToString(dest, args...);
}
template<typename... Args> void appendToString(String &dest, double data, Args... args)
{
	dest += std::to_string(data);
	appendToString(dest, args...);
}
template<typename T, typename... Args> void appendToString(String &dest, Args... args)
{
	int tmp[] = {(appendToString(dest, args))...};
	static_cast<void>(tmp);
}

} // namespace fer