#pragma once

#include "Core.hpp"

namespace fer::utils
{

inline bool startsWith(StringRef src, StringRef term) { return src.rfind(term, 0) == 0; }

// Also trims the spaces for each split
Vector<StringRef> stringDelim(StringRef str, StringRef delim);

// Convert special characters in string (\n, \t, ...) to raw (\\n, \\t, ...)
// and vice versa
String toRawString(StringRef data);
String fromRawString(StringRef data);

String vecToStr(Span<StringRef> items);
String vecToStr(Span<String> items);

void removeBackSlash(String &s);
String viewBackSlash(StringRef data);

inline void appendToString(String &dest) {}

inline void appendToString(String &dest, bool data) { dest += data ? "(true)" : "(false)"; }
inline void appendToString(String &dest, char data) { dest += data; }
inline void appendToString(String &dest, uint8_t data) { dest += std::to_string(data); }
inline void appendToString(String &dest, uint16_t data) { dest += std::to_string(data); }
inline void appendToString(String &dest, int64_t data) { dest += std::to_string(data); }
inline void appendToString(String &dest, size_t data) { dest += std::to_string(data); }
inline void appendToString(String &dest, int data) { dest += std::to_string(data); }
inline void appendToString(String &dest, float data) { dest += std::to_string(data); }
inline void appendToString(String &dest, double data) { dest += std::to_string(data); }
inline void appendToString(String &dest, char *data) { dest += data; }
inline void appendToString(String &dest, const char *data) { dest += data; }
inline void appendToString(String &dest, StringRef data) { dest += data; }
inline void appendToString(String &dest, const String &data) { dest += data; }

template<typename... Args> void appendToString(String &dest, Args... args)
{
	int tmp[] = {(appendToString(dest, args), 0)...};
	static_cast<void>(tmp);
}
template<typename... Args> String toString(Args... args)
{
	String dest;
	appendToString(dest, std::forward<Args>(args)...);
	return dest;
}

#if defined(FER_OS_WINDOWS)
// Windows' string to wstring functions
WString toWString(StringRef data);
#endif

} // namespace fer::utils