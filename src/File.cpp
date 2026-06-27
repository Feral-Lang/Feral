#include "File.hpp"

namespace fer
{

Status<bool> File::readFile(const char *file, String &data)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(file, "r");
    if(fp == NULL) return Status(false, "Error: failed to open source file: ", file);

    while((read = getline(&line, &len, fp)) != -1) data += line;

    fclose(fp);
    if(line) free(line);

    return Status(true);
}

File::File(const char *path, bool isVirt) : path(path), isVirt(isVirt) {}

Status<bool> File::read()
{
    if(isVirt) return Status(false, "Cannot read a virtual file: ", path);
    Status<bool> res = readFile(path.c_str(), data);
    return res;
}

bool File::set(String &&data)
{
    if(!isVirt) return false;
    this->appendLocs.clear();
    this->appendLocs.push_back(0);
    this->data = std::move(data);
    return true;
}
bool File::append(StringRef data)
{
    if(!isVirt || data.empty()) return false;
    this->appendLocs.push_back(this->data.size());
    this->data += data;
    return true;
}

StringRef File::getAppendData(size_t index) const
{
    if(index >= appendLocs.size()) return "";
    size_t start = appendLocs[index];
    size_t end   = data.size();
    if(index < appendLocs.size() - 1) end = appendLocs[index + 1];
    return StringRef(data.begin() + appendLocs[index], data.begin() + end);
}

} // namespace fer

#if defined(FER_OS_WINDOWS)
// getdelim and getline functions from NetBSD libnbcompat:
// https://github.com/archiecobbs/libnbcompat

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
ssize_t getdelim(char **buf, size_t *bufsiz, int delimiter, FILE *fp)
{
    char *ptr, *eptr;

    if(*buf == NULL || *bufsiz == 0) {
        *bufsiz = BUFSIZ;
        if((*buf = (char *)malloc(*bufsiz)) == NULL) return -1;
    }

    for(ptr = *buf, eptr = *buf + *bufsiz;;) {
        int c = fgetc(fp);
        if(c == -1) {
            if(feof(fp)) {
                ssize_t diff = (ssize_t)(ptr - *buf);
                if(diff != 0) {
                    *ptr = '\0';
                    return diff;
                }
            }
            return -1;
        }
        *ptr++ = c;
        if(c == delimiter) {
            *ptr = '\0';
            return ptr - *buf;
        }
        if(ptr + 2 >= eptr) {
            char *nbuf;
            size_t nbufsiz = *bufsiz * 2;
            ssize_t d      = ptr - *buf;
            if((nbuf = (char *)realloc(*buf, nbufsiz)) == NULL) return -1;
            *buf    = nbuf;
            *bufsiz = nbufsiz;
            eptr    = nbuf + nbufsiz;
            ptr     = nbuf + d;
        }
    }
    return 0;
}

ssize_t getline(char **buf, size_t *bufsiz, FILE *fp) { return getdelim(buf, bufsiz, '\n', fp); }
#endif