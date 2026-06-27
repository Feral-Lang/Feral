#pragma once

#include "Allocator.hpp"
#include "Status.hpp"

#if defined(FER_OS_WINDOWS)
FER_API ssize_t getdelim(char **buf, size_t *bufsiz, int delimiter, FILE *fp);
FER_API ssize_t getline(char **buf, size_t *bufsiz, FILE *fp);
#endif

namespace fer
{

// To create and manage files (including virtual like `<eval>`)
// as well as provide error messages using file locations.
class FER_API File : public IAllocated
{
    String path;
    // All file contents are stored in this.
    String data;
    // For virtual files, locations where append() was called.
    // Using this, say, the last appended section of the data string can be retrieved.
    Vector<size_t> appendLocs;
    bool isVirt; // isVirtual

public:
    File(const char *path, bool isVirt);

    Status<bool> read();

    bool set(String &&data);
    // Only for virtual files.
    bool append(StringRef data);
    StringRef getAppendData(size_t index) const;

    static Status<bool> readFile(const char *file, String &data);

    inline StringRef getPath() const { return path; }
    inline const char *getPathCStr() const { return path.c_str(); }
    inline StringRef getData() const { return data; }
    inline bool isVirtual() const { return isVirt; }
    inline StringRef getLastAppendData() const { return getAppendData(appendLocs.size() - 1); }

    inline bool emptyData() const { return data.empty(); }
    // appendLocs is never empty.
    inline size_t sizeData() const { return data.size(); }
    inline size_t sizeAppendLocs() const { return appendLocs.size(); }
};

} // namespace fer