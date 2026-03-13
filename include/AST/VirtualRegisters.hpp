#pragma once

#include "Core.hpp"

namespace fer::ast
{

using namespace core;

class VRegManager
{
    Vector<Vector<StringRef>> data;
    Vector<Vector<size_t>> blocks;

public:
    inline void pushFunc()
    {
        data.push_back({});
        blocks.push_back({});
    }
    inline void popFunc()
    {
        data.pop_back();
        blocks.pop_back();
    }

    inline void pushBlk()
    {
        if(data.empty()) return;
        blocks.back().push_back(data.back().size());
    }
    inline void popBlk()
    {
        if(data.empty()) return;
        while(data.back().size() > blocks.back().size()) data.back().pop_back();
        blocks.pop_back();
    }

    inline bool pushName(StringRef name)
    {
        if(data.empty()) return false;
        data.back().push_back(name);
        return true;
    }

    inline size_t getIndex(StringRef name)
    {
        if(data.empty()) return -1;
        auto loc = std::find(data.back().begin(), data.back().end(), name);
        if(loc == data.back().end()) return -1;
        return loc - data.back().begin();
    }

    inline size_t getLastIndex()
    {
        if(data.empty()) return -1;
        return data.back().size() - 1;
    }
};

} // namespace fer::ast