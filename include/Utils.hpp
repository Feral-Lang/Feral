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
void appendRawString(String &res, StringRef from);

String vecToStr(Span<StringRef> items);

} // namespace fer